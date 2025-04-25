#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::string output_directory = "../../src";
std::string output_filename = "fonts.hpp";
bool flip_vertical = true;

const int GLYPH_SIZE = 24;
const int PADDING = 1;
const int MAX_ATLAS_SIZE = 2048;

struct CodepointRange {
    int begin;
    int end;
};

const std::vector<CodepointRange> ranges_en = {{0x20, 0x7E}};
const std::vector<CodepointRange> ranges_ua = {{0x0400, 0x04FF}};
const std::vector<CodepointRange> ranges_jp = {
    {0x3040, 0x309F}, {0x30A0, 0x30FF}, {0x4E00, 0x9FAF}};

std::vector<int> get_codepoints(const std::string &lang) {
    std::vector<int> cps;
    const auto &ranges = (lang == "en")   ? ranges_en
                         : (lang == "ua") ? ranges_ua
                                          : ranges_jp;
    for (auto r : ranges)
        for (int c = r.begin; c <= r.end; ++c)
            cps.push_back(c);
    return cps;
}

struct GlyphSource {
    std::string font_path;
    stbtt_fontinfo font;
    std::vector<unsigned char> buffer;
};

int main(int argc, char **argv) {
    struct FontLang {
        std::string path;
        std::string lang;
    };
    std::vector<FontLang> font_inputs;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.starts_with("--")) {
            size_t sep = arg.find_last_of('-');
            if (sep != std::string::npos) {
                std::string path = arg.substr(2, sep - 2);
                std::string lang = arg.substr(sep + 1);
                font_inputs.push_back({path, lang});
            }
        } else if (arg == "--flip-vertical") {
            flip_vertical = true;
        }
    }

    std::vector<GlyphSource> glyph_sources_storage;
    std::map<int, const GlyphSource *> glyph_sources;
    std::set<std::string> langs;

    for (const auto &entry : font_inputs) {
        langs.insert(entry.lang);

        std::ifstream font_file(entry.path, std::ios::binary);
        if (!font_file) {
            std::cerr << "Error: failed to open font file: " << entry.path
                      << "\n";
            return 1;
        }

        GlyphSource source;
        source.font_path = entry.path;
        source.buffer = std::vector<unsigned char>(
            (std::istreambuf_iterator<char>(font_file)), {});
        if (!stbtt_InitFont(&source.font, source.buffer.data(), 0)) {
            std::cerr << "Error: failed to init font: " << entry.path << "\n";
            return 1;
        }

        glyph_sources_storage.push_back(std::move(source));
        GlyphSource *ptr = &glyph_sources_storage.back();
        for (int cp : get_codepoints(entry.lang))
            glyph_sources[cp] = ptr;
    }

    std::vector<int> codepoints;
    for (const auto &[cp, _] : glyph_sources)
        codepoints.push_back(cp);

    std::sort(codepoints.begin(), codepoints.end());

    for (int cp : codepoints)
        std::cout << static_cast<char>(cp) << " ";

    std::vector<unsigned char> atlas_bitmap;
    std::vector<std::string> glyph_entries;
    bool success = false;

    for (int size = 128; size <= MAX_ATLAS_SIZE; size *= 2) {
        int atlas_width = size, atlas_height = size;
        std::vector<unsigned char> test_bitmap(atlas_width * atlas_height, 0);
        std::vector<std::string> test_glyphs;

        int x = 0, y = 0, max_row = 0;
        bool fits = true;

        std::cout << "[debug] rendering " << codepoints.size()
                  << " glyphs into atlas " << atlas_width << "x" << atlas_height
                  << "\n";

        for (int cp : codepoints) {
            if (!glyph_sources.contains(cp))
                continue;
            const auto *src = glyph_sources.at(cp);

            float scale =
                stbtt_ScaleForPixelHeight(&src->font, (float)GLYPH_SIZE);
            int w, h, xoff, yoff;
            unsigned char *bitmap = stbtt_GetCodepointBitmap(
                &src->font, 0, scale, cp, &w, &h, &xoff, &yoff);

            if (x + w + PADDING > atlas_width) {
                x = 0;
                y += max_row + PADDING;
                max_row = 0;
            }
            if (y + h + PADDING > atlas_height) {
                fits = false;
                stbtt_FreeBitmap(bitmap, nullptr);
                break;
            }

            for (int j = 0; j < h; ++j) {
                for (int i = 0; i < w; ++i) {
                    int dest_y = flip_vertical ? (y + (h - 1 - j)) : (y + j);
                    test_bitmap[dest_y * atlas_width + (x + i)] =
                        bitmap[j * w + i];
                }
            }

            scale = stbtt_ScaleForPixelHeight(&src->font, (float)GLYPH_SIZE);
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&src->font, cp, &adv, &lsb);
            int scaled_advance = std::round(adv * scale);

            test_glyphs.push_back(
                "    {" + std::to_string(x) + ", " + std::to_string(y) + ", " +
                std::to_string(w) + ", " + std::to_string(h) + ", " +
                std::to_string(scaled_advance) + ", " + // ⬅️ тут
                std::to_string(xoff) + ", " + std::to_string(yoff) + ", " +
                std::to_string(cp) + "},");

            x += w + PADDING;
            if (h > max_row)
                max_row = h;
            stbtt_FreeBitmap(bitmap, nullptr);
        }

        if (fits) {
            atlas_bitmap = std::move(test_bitmap);
            glyph_entries = std::move(test_glyphs);

            fs::path out_path = fs::path(output_directory) / output_filename;
            std::ofstream out(out_path);

            out << "#pragma once\n\n";
            out << "enum class FontLanguage { en, ua, jp };\n";
            out << "constexpr FontLanguage supported_languages[] = { ";
            for (const auto &lang : langs)
                out << "FontLanguage::" << lang << ", ";
            out << "};\n\n";

            out << "constexpr int FONT_ATLAS_WIDTH = " << atlas_width << ";\n";
            out << "constexpr int FONT_ATLAS_HEIGHT = " << atlas_height
                << ";\n";
            out << "constexpr unsigned char font_bitmap[FONT_ATLAS_WIDTH * "
                   "FONT_ATLAS_HEIGHT] = {\n";
            for (size_t i = 0; i < atlas_bitmap.size(); ++i)
                out << (int)atlas_bitmap[i] << ((i % 32 == 31) ? ",\n" : ", ");
            out << "};\n\n";

            out << "struct GlyphInfo { int x, y, w, h, advance, offset_x, "
                   "offset_y, codepoint; };\n";
            out << "constexpr GlyphInfo font_glyphs[] = {\n";
            for (const auto &entry : glyph_entries)
                out << entry << "\n";
            out << "};\n";

            std::cout << "[info] Generated " << out_path << " with "
                      << glyph_entries.size() << " glyphs in " << atlas_width
                      << "x" << atlas_height << " atlas.\n";
            success = true;
            break;
        }
    }

    if (!success) {
        std::cerr << "Error: Unable to fit glyphs in max atlas size."
                  << std::endl;
        return 1;
    }

    return 0;
}
