#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::string output_directory = "../../src";
std::string output_filename = "fonts.hpp";

const int GLYPH_SIZE = 32;
const int ATLAS_WIDTH = 512;
const int ATLAS_HEIGHT = 512;

struct CodepointRange {
    int begin;
    int end;
};

const std::vector<CodepointRange> ranges_en = {{0x20, 0x7E}};
const std::vector<CodepointRange> ranges_ua = {{0x0400, 0x04FF}};
const std::vector<CodepointRange> ranges_ja = {
    {0x3040, 0x309F}, {0x30A0, 0x30FF}, {0x4E00, 0x9FAF}};

std::vector<int> collect_codepoints(const std::set<std::string> &langs) {
    std::vector<int> cps;
    auto append_range = [&](const auto &ranges) {
        for (auto r : ranges)
            for (int c = r.begin; c <= r.end; ++c)
                cps.push_back(c);
    };
    if (langs.contains("en"))
        append_range(ranges_en);
    if (langs.contains("ua"))
        append_range(ranges_ua);
    if (langs.contains("ja"))
        append_range(ranges_ja);
    return cps;
}

int main(int argc, char **argv) {
    std::string font_path;
    std::set<std::string> langs;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            font_path = argv[++i];
            if (!font_path.ends_with(".ttf"))
                font_path += ".ttf";
        } else if (arg.starts_with("--languages-")) {
            std::string langs_arg = arg.substr(12);
            size_t pos = 0;
            while ((pos = langs_arg.find('-')) != std::string::npos) {
                langs.insert(langs_arg.substr(0, 2));
                langs_arg = langs_arg.substr(pos + 1);
            }
            if (!langs_arg.empty())
                langs.insert(langs_arg.substr(0, 2));
        }
    }

    if (!langs.contains("en")) {
        std::cerr << "Error: English (en) must be included.\n";
        return 1;
    }

    if (font_path.empty()) {
        std::vector<fs::path> ttf_files;
        for (const auto &entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().extension() == ".ttf") {
                ttf_files.push_back(entry.path());
            }
        }
        if (ttf_files.size() == 1) {
            font_path = ttf_files[0].string();
            std::cout << "[info] Using font: " << font_path << "\n";
        } else {
            std::cerr << "Error: specify font with -f or ensure only one .ttf "
                         "file exists.\n";
            return 1;
        }
    }

    std::ifstream font_file(font_path, std::ios::binary);
    if (!font_file) {
        std::cerr << "Error: failed to open font file." << std::endl;
        return 1;
    }

    std::vector<unsigned char> font_buffer(
        (std::istreambuf_iterator<char>(font_file)), {});
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, font_buffer.data(), 0)) {
        std::cerr << "Error: failed to initialize font." << std::endl;
        return 1;
    }

    std::vector<int> codepoints = collect_codepoints(langs);
    unsigned char atlas[ATLAS_WIDTH * ATLAS_HEIGHT] = {};
    std::vector<std::string> glyph_entries;

    int x = 0, y = 0, max_row_height = 0;
    for (int cp : codepoints) {
        int w, h, xoff, yoff;
        float scale = stbtt_ScaleForPixelHeight(&font, (float)GLYPH_SIZE);
        unsigned char *bitmap =
            stbtt_GetCodepointBitmap(&font, 0, scale, cp, &w, &h, &xoff, &yoff);

        if (x + w >= ATLAS_WIDTH) {
            x = 0;
            y += max_row_height;
            max_row_height = 0;
        }
        if (y + h >= ATLAS_HEIGHT) {
            std::cerr << "Error: Atlas overflow. Increase dimensions."
                      << std::endl;
            stbtt_FreeBitmap(bitmap, nullptr);
            return 1;
        }

        for (int j = 0; j < h; ++j)
            for (int i = 0; i < w; ++i)
                atlas[(y + j) * ATLAS_WIDTH + (x + i)] = bitmap[j * w + i];

        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);

        glyph_entries.push_back(
            "    {" + std::to_string(x) + ", " + std::to_string(y) + ", " +
            std::to_string(w) + ", " + std::to_string(h) + ", " +
            std::to_string(advance) + ", " + std::to_string(xoff) + ", " +
            std::to_string(yoff) + ", " + std::to_string(cp) + "},");

        x += w + 1;
        if (h > max_row_height)
            max_row_height = h;
        stbtt_FreeBitmap(bitmap, nullptr);
    }

    fs::path out_path = fs::path(output_directory) / output_filename;
    std::ofstream out(out_path);

    out << "#pragma once\n\n";
    out << "enum class FontLanguage { en, ua, ja };\n";
    out << "constexpr FontLanguage supported_languages[] = { ";
    for (const auto &lang : langs) {
        out << "FontLanguage::" << lang << ", ";
    }
    out << "};\n\n";

    out << "constexpr int FONT_ATLAS_WIDTH = " << ATLAS_WIDTH << ";\n";
    out << "constexpr int FONT_ATLAS_HEIGHT = " << ATLAS_HEIGHT << ";\n";
    out << "constexpr unsigned char font_bitmap["
        << (ATLAS_WIDTH * ATLAS_HEIGHT) << "] = {\n";
    for (int i = 0; i < ATLAS_WIDTH * ATLAS_HEIGHT; ++i) {
        out << (int)atlas[i] << ((i % 32 == 31) ? ",\n" : ", ");
    }
    out << "};\n\n";

    out << "struct GlyphInfo { int x, y, w, h, advance, offset_x, offset_y, "
           "codepoint; };\n";
    out << "constexpr GlyphInfo font_glyphs[] = {\n";
    for (const auto &entry : glyph_entries)
        out << entry << "\n";
    out << "};\n";

    std::cout << "[info] Generated " << out_path << " with "
              << glyph_entries.size() << " glyphs.\n";
    return 0;
}
