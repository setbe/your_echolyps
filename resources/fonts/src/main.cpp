#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#include "jouyou_kanji.hpp"

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
constexpr int PADDING = 0;
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
const std::vector<CodepointRange> ranges_en = {
    {0x0020, 0x007E}, // Basic Latin
    // {0x00A0, 0x00FF}, // Latin-1 Supplement
    // {0x0100, 0x017F}, // Latin Extended-A
    // {0x0180, 0x024F}  // Latin Extended-B
};
const std::vector<CodepointRange> ranges_ua = {
    {0x0410, 0x0429}, // А-Щ
    {0x042E, 0x0449}, // ЮЯа-щ
    {0x044C, 0x044C}, // ь
    {0x044E, 0x044F}, // юя
    {0x0456, 0x0457}, // ії
};
std::vector<CodepointRange> ranges_jp = {
    {0x3040, 0x309F}, // Hiragana
    {0x30A0, 0x30FF}, // Katakana
    // {0x4E00, 0x7FFF}  // CJK (cutted)
    // {0x4E00, 0x9FAF} // CJK Unified Ideographs (~20k)
};

inline std::vector<int> get_codepoints(const std::string &lang) {
    std::vector<int> cps;

    auto add_range = [&](const CodepointRange *ranges, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            for (int c = ranges[i].begin; c <= ranges[i].end; ++c) {
                cps.push_back(c);
            }
        }
    };

    if (lang == "en") {
        add_range(ranges_en.data(), ranges_en.size());
    } else if (lang == "ua") {
        add_range(ranges_ua.data(), ranges_ua.size());
    } else if (lang == "jp") {
        add_range(ranges_jp.data(), ranges_jp.size());
        cps.insert(cps.end(), jouyou_kanji_codepoints,
                   jouyou_kanji_codepoints +
                       sizeof(jouyou_kanji_codepoints) / sizeof(int));
    } else {
        throw std::runtime_error("Unknown language: " + lang);
    }

    return cps;
}

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

    for (const auto &src : glyph_sources_storage) {
        for (int cp : get_codepoints(src.lang)) {
            int glyph_index = stbtt_FindGlyphIndex(&src.font, cp);
            if (glyph_index != 0)
                glyph_sources[cp] = &src;
        }
    }
}

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

        for (int cp : codepoints) {
            const auto *src = glyph_sources.at(cp);
            float scale =
                stbtt_ScaleForPixelHeight(&src->font, (float)glyph_size);

            int w, h, xoff, yoff;
            unsigned char *bitmap = stbtt_GetCodepointBitmap(
                &src->font, 0, scale, cp, &w, &h, &xoff, &yoff);

            if (!bitmap) {
                int adv, lsb;
                stbtt_GetCodepointHMetrics(&src->font, cp, &adv, &lsb);
                int scaled_advance = std::round(adv * scale);

                test_glyphs.push_back("    {0, 0, 0, 0, " +
                                      std::to_string(scaled_advance) +
                                      ", 0, 0, " + std::to_string(cp) + "},");
                continue;
            }

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

            for (int j = 0; j < h; ++j)
                for (int i = 0; i < w; ++i) {
                    int dest_y = flip_vertical ? (y + (h - 1 - j)) : (y + j);
                    test_bitmap[dest_y * atlas_width + (x + i)] =
                        bitmap[j * w + i];
                }

            int adv, lsb;
            stbtt_GetCodepointHMetrics(&src->font, cp, &adv, &lsb);
            int scaled_advance = std::round(adv * scale);

            if (scaled_advance < w + 1) {
                scaled_advance = w + 3;
            }

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

void write_output(const std::vector<unsigned char> &atlas_bitmap,
                  const std::vector<std::string> &glyph_entries,
                  const std::set<std::string> &langs, int atlas_width,
                  int atlas_height) {
    fs::path out_path = fs::path(output_directory) / output_filename;
    std::ofstream out(out_path);
    if (!out)
        throw std::runtime_error("Failed to open output file: " +
                                 out_path.string());

    // Step 1: Bit compression (8 pixels per byte)
    std::vector<unsigned char> bit_compressed;
    unsigned char current_byte = 0;
    int bit_pos = 0;

    for (auto pixel : atlas_bitmap) {
        if (pixel >= 128)
            current_byte |= (1 << (7 - bit_pos));
        bit_pos++;
        if (bit_pos == 8) {
            bit_compressed.push_back(current_byte);
            current_byte = 0;
            bit_pos = 0;
        }
    }
    if (bit_pos != 0)
        bit_compressed.push_back(current_byte);

    // Step 2: Zero-RLE compression on bit-compressed data
    std::vector<unsigned char> compressed_bitmap;
    for (size_t i = 0; i < bit_compressed.size();) {
        if (bit_compressed[i] == 0) {
            size_t run = 1;
            while (i + run < bit_compressed.size() &&
                   bit_compressed[i + run] == 0 && run < 255)
                run++;
            compressed_bitmap.push_back(0);
            compressed_bitmap.push_back(static_cast<unsigned char>(run));
            i += run;
        } else {
            compressed_bitmap.push_back(bit_compressed[i]);
            i++;
        }
    }

    // Output header file
    out << "#pragma once\n\n";
    out << "constexpr int FONT_ATLAS_WIDTH = " << atlas_width << ";\n";
    out << "constexpr int FONT_ATLAS_HEIGHT = " << atlas_height << ";\n";
    out << "constexpr int FONT_ASCENT = " << font_ascent << ";\n\n";

    out << "constexpr unsigned char compressed_font_bitmap[] = {\n";
    for (size_t i = 0; i < compressed_bitmap.size(); ++i)
        out << (int)compressed_bitmap[i] << ((i % 32 == 31) ? ",\n" : ", ");
    out << "};\n\n";

    out << "inline void decompress_font_bitmap(unsigned char *out) noexcept "
           "{\n";
    out << "    unsigned char *ptr = out;\n";
    out << "    unsigned i = 0;\n";
    out << "    while (i < sizeof(compressed_font_bitmap)) {\n";
    out << "        unsigned char byte = compressed_font_bitmap[i++];\n";
    out << "        if (byte == 0) {\n";
    out << "            if (i >= sizeof(compressed_font_bitmap)) break;\n";
    out << "            unsigned char count = compressed_font_bitmap[i++];\n";
    out << "            for (int j = 0; j < count; ++j) {\n";
    out << "                for (int b = 0; b < 8; ++b) {\n";
    out << "                    *ptr++ = 0;\n";
    out << "                }\n";
    out << "            }\n";
    out << "        } else {\n";
    out << "            for (int b = 0; b < 8; ++b) {\n";
    out << "                *ptr++ = (byte & (1 << (7 - b))) ? 255 : 0;\n";
    out << "            }\n";
    out << "        }\n";
    out << "    }\n";
    out << "}\n\n";

    out << "struct GlyphInfo { int x, y, w, h, advance, offset_x, offset_y, "
           "codepoint; };\n";
    out << "constexpr GlyphInfo font_glyphs[] = {\n";
    for (const auto &entry : glyph_entries)
        out << entry << "\n";
    out << "};\n";
}
int main(int argc, char **argv) {
    try {
        std::vector<std::pair<std::string, std::string>> font_inputs;

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

        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&glyph_sources_storage.front().font, &ascent,
                              &descent, &line_gap);
        font_ascent =
            std::round(ascent * stbtt_ScaleForPixelHeight(
                                    &glyph_sources_storage.front().font,
                                    (float)glyph_size));

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