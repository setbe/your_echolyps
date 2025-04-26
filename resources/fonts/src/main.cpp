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

// Global configuration
int glyph_size = -1;
int font_ascent = 0; // Will be calculated after loading font
constexpr int PADDING = 1;
constexpr int MAX_ATLAS_SIZE = 2048;

std::string output_directory = "../../src";
std::string output_filename = "fonts.hpp";
bool flip_vertical = true;

// Codepoint range definition
struct CodepointRange {
    int begin, end;
};

// Font source information
struct GlyphSource {
    std::string font_path;
    std::string lang;
    stbtt_fontinfo font;
    std::vector<unsigned char> buffer;
};

// Supported language codepoint ranges
const std::vector<CodepointRange> ranges_la = {
    {0x0020, 0x007E}, // Basic Latin
    // {0x00A0, 0x00FF}, // Latin-1 Supplement
    // {0x0100, 0x017F}, // Latin Extended-A
    // {0x0180, 0x024F}  // Latin Extended-B
};
const std::vector<CodepointRange> ranges_cy = {
    {0x0410, 0x0429}, // А-Щ
    {0x042E, 0x0449}, // ЮЯа-щ
    {0x044C, 0x044C}, // ь
    {0x044E, 0x044F}, // юя
    {0x0456, 0x0457}, // ії
};
const std::vector<CodepointRange> ranges_jp = {
    {0x3040, 0x309F}, // Hiragana
    {0x30A0, 0x30FF}, // Katakana
    {0x4E00, 0x9FAF}  // CJK Unified Ideographs
};

// Get codepoint ranges based on language
const std::vector<CodepointRange> *get_ranges(const std::string &lang) {
    if (lang == "la")
        return &ranges_la;
    if (lang == "cy")
        return &ranges_cy;
    if (lang == "jp")
        return &ranges_jp;
    throw std::runtime_error("Unknown language: " + lang);
}

// Collect all codepoints for a given language
std::vector<int> get_codepoints(const std::string &lang) {
    std::vector<int> cps;
    const auto *ranges = get_ranges(lang);
    for (auto r : *ranges)
        for (int c = r.begin; c <= r.end; ++c)
            cps.push_back(c);
    return cps;
}

// Load all fonts and build glyph mapping
void load_fonts(
    const std::vector<std::pair<std::string, std::string>> &font_inputs,
    std::vector<GlyphSource> &glyph_sources_storage,
    std::map<int, const GlyphSource *> &glyph_sources,
    std::set<std::string> &langs) {
    for (const auto &entry : font_inputs) {
        langs.insert(entry.second);

        std::ifstream font_file(entry.first, std::ios::binary);
        if (!font_file)
            throw std::runtime_error("Failed to open font file: " +
                                     entry.first);

        GlyphSource source;
        source.font_path = entry.first;
        source.lang = entry.second;
        source.buffer = std::vector<unsigned char>(
            (std::istreambuf_iterator<char>(font_file)), {});
        if (!stbtt_InitFont(&source.font, source.buffer.data(), 0))
            throw std::runtime_error("Failed to initialize font: " +
                                     entry.first);

        glyph_sources_storage.push_back(std::move(source));
    }

    // Assign codepoints to font sources (last font wins in case of conflicts)
    for (const auto &src : glyph_sources_storage) {
        for (int cp : get_codepoints(src.lang)) {
            int glyph_index = stbtt_FindGlyphIndex(&src.font, cp);
            if (glyph_index != 0)
                glyph_sources[cp] = &src;
        }
    }
}

// Try to pack all glyphs into texture atlas
bool try_pack_glyphs(const std::vector<int> &codepoints,
                     const std::map<int, const GlyphSource *> &glyph_sources,
                     std::vector<unsigned char> &atlas_bitmap,
                     std::vector<std::string> &glyph_entries, int &out_width,
                     int &out_height) {
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
            const auto *src = glyph_sources.at(cp);
            float scale =
                stbtt_ScaleForPixelHeight(&src->font, (float)glyph_size);

            int w, h, xoff, yoff;
            unsigned char *bitmap = stbtt_GetCodepointBitmap(
                &src->font, 0, scale, cp, &w, &h, &xoff, &yoff);

            if (!bitmap) {
                // Invisible glyph (like space) — add advance but no bitmap
                int adv, lsb;
                stbtt_GetCodepointHMetrics(&src->font, cp, &adv, &lsb);
                int scaled_advance = std::round(adv * scale);

                test_glyphs.push_back("    {" + std::to_string(x) + ", " +
                                      std::to_string(y) + ", 0, 0, " +
                                      std::to_string(scaled_advance) +
                                      ", 0, 0, " + std::to_string(cp) + "},");
                x += PADDING;
                continue;
            }

            // Atlas wrapping check
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

            // Copy bitmap into atlas
            for (int j = 0; j < h; ++j)
                for (int i = 0; i < w; ++i) {
                    int dest_y = flip_vertical ? (y + (h - 1 - j)) : (y + j);
                    test_bitmap[dest_y * atlas_width + (x + i)] =
                        bitmap[j * w + i];
                }

            // Metrics
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&src->font, cp, &adv, &lsb);
            int scaled_advance = std::round(adv * scale);

            test_glyphs.push_back(
                "    {" + std::to_string(x) + ", " + std::to_string(y) + ", " +
                std::to_string(w) + ", " + std::to_string(h) + ", " +
                std::to_string(scaled_advance) + ", " + std::to_string(xoff) +
                ", " + std::to_string(yoff) + ", " + std::to_string(cp) + "},");

            x += w + PADDING;
            if (h > max_row)
                max_row = h;

            stbtt_FreeBitmap(bitmap, nullptr);
        }

        if (fits) {
            atlas_bitmap = std::move(test_bitmap);
            glyph_entries = std::move(test_glyphs);
            out_width = atlas_width;
            out_height = atlas_height;
            return true;
        }
    }
    return false;
}

// Output header file
void write_output(const std::vector<unsigned char> &atlas_bitmap,
                  const std::vector<std::string> &glyph_entries,
                  const std::set<std::string> &langs, int atlas_width,
                  int atlas_height) {
    fs::path out_path = fs::path(output_directory) / output_filename;
    std::ofstream out(out_path);
    if (!out)
        throw std::runtime_error("Failed to open output file: " +
                                 out_path.string());

    out << "#pragma once\n\n";
    out << "enum class FontLanguage { la, cy, jp };\n";
    out << "constexpr FontLanguage supported_languages[] = { ";
    for (const auto &lang : langs)
        out << "FontLanguage::" << lang << ", ";
    out << "};\n\n";

    out << "constexpr int FONT_ATLAS_WIDTH = " << atlas_width << ";\n";
    out << "constexpr int FONT_ATLAS_HEIGHT = " << atlas_height << ";\n";
    out << "constexpr int FONT_ASCENT = " << font_ascent << ";\n\n";

    out << "constexpr unsigned char font_bitmap[FONT_ATLAS_WIDTH * "
           "FONT_ATLAS_HEIGHT] = {\n";
    for (size_t i = 0; i < atlas_bitmap.size(); ++i)
        out << (int)atlas_bitmap[i] << ((i % 32 == 31) ? ",\n" : ", ");
    out << "};\n\n";

    out << "struct GlyphInfo { int x, y, w, h, advance, offset_x, offset_y, "
           "codepoint; };\n";
    out << "constexpr GlyphInfo font_glyphs[] = {\n";
    for (const auto &entry : glyph_entries)
        out << entry << "\n";
    out << "};\n";
}

// Entry point
int main(int argc, char **argv) {
    try {
        std::vector<std::pair<std::string, std::string>> font_inputs;

        // Parse command line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.starts_with("--size=")) {
                glyph_size = std::stoi(arg.substr(7));
                if (glyph_size <= 0)
                    throw std::runtime_error("Invalid font size specified.");
            } else if (arg.starts_with("--")) {
                size_t sep = arg.find_last_of('-');
                if (sep != std::string::npos) {
                    std::string path = arg.substr(2, sep - 2);
                    std::string lang = arg.substr(sep + 1);
                    font_inputs.emplace_back(path, lang);
                }
            } else if (arg == "--flip-vertical") {
                flip_vertical = true;
            }
        }

        if (glyph_size <= 0)
            throw std::runtime_error("Font size (--size=N) must be specified.");

        std::vector<GlyphSource> glyph_sources_storage;
        std::map<int, const GlyphSource *> glyph_sources;
        std::set<std::string> langs;

        load_fonts(font_inputs, glyph_sources_storage, glyph_sources, langs);

        if (glyph_sources_storage.empty())
            throw std::runtime_error("No fonts loaded.");

        // Calculate font ascent
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&glyph_sources_storage.front().font, &ascent,
                              &descent, &line_gap);
        float scale = stbtt_ScaleForPixelHeight(
            &glyph_sources_storage.front().font, (float)glyph_size);
        font_ascent = std::round(ascent * scale);

        std::cout << "[debug] font ascent (scaled) = " << font_ascent << "\n";

        std::vector<int> codepoints;
        for (const auto &[cp, _] : glyph_sources)
            codepoints.push_back(cp);

        std::sort(codepoints.begin(), codepoints.end());

        std::vector<unsigned char> atlas_bitmap;
        std::vector<std::string> glyph_entries;
        int atlas_width = 0, atlas_height = 0;

        if (!try_pack_glyphs(codepoints, glyph_sources, atlas_bitmap,
                             glyph_entries, atlas_width, atlas_height))
            throw std::runtime_error(
                "Unable to fit glyphs in max atlas size (try less font size or "
                "remove languages).");

        write_output(atlas_bitmap, glyph_entries, langs, atlas_width,
                     atlas_height);

        std::cout << "[info] Generated " << output_directory << "/"
                  << output_filename << " with " << glyph_entries.size()
                  << " glyphs in " << atlas_width << "x" << atlas_height
                  << " atlas.\n";

        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "[error] Exception: " << ex.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[error] Unknown exception.\n";
        return 1;
    }
}
