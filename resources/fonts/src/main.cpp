// fonts_gen.cpp
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../../src/external/miniz.hpp"
#include "external/stb_truetype.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
constexpr int PADDING = 1;
constexpr int MAX_ATLAS = 2048;

struct CodepointRange {
    int begin, end;
};
struct GlyphSource {
    std::string path, lang;
    stbtt_fontinfo font;
    std::vector<unsigned char> buf;
};

static const std::vector<CodepointRange> ranges_en = {{0x20, 0x7E}};
static const std::vector<CodepointRange> ranges_ua = {{0x410, 0x429},
                                                      {0x42E, 0x449},
                                                      {0x44C, 0x44C},
                                                      {0x44E, 0x44F},
                                                      {0x456, 0x457}};

std::vector<int> get_codepoints(const std::string &lang) {
    std::vector<int> cps;
    auto add = [&](auto &R) {
        for (auto &r : R)
            for (int c = r.begin; c <= r.end; ++c)
                cps.push_back(c);
    };
    if (lang == "en")
        add(ranges_en);
    else if (lang == "ua")
        add(ranges_ua);
    else
        throw std::runtime_error("Unknown lang: " + lang);
    return cps;
}

void load_fonts(const std::vector<std::pair<std::string, std::string>> &in,
                std::vector<GlyphSource> &storage,
                std::map<int, const GlyphSource *> &out) {
    for (auto &[path, lang] : in) {
        std::ifstream f(path, std::ios::binary);
        if (!f)
            throw std::runtime_error("Can't open " + path);
        GlyphSource S{path, lang};
        S.buf.assign(std::istreambuf_iterator<char>(f), {});
        if (!stbtt_InitFont(&S.font, S.buf.data(), 0))
            throw std::runtime_error("stbtt_InitFont failed: " + path);
        storage.push_back(std::move(S));
    }
    for (auto &src : storage) {
        for (int cp : get_codepoints(src.lang)) {
            int gi = stbtt_FindGlyphIndex(&src.font, cp);
            if (gi)
                out[cp] = &src;
        }
    }
}

bool pack_glyphs(const std::vector<int> &cps,
                 const std::map<int, const GlyphSource *> &srcs,
                 std::vector<unsigned char> &atlas,
                 std::vector<std::array<int, 8>> &info, int &W, int &H,
                 int size, bool flip) {
    for (int s = 128; s <= MAX_ATLAS; s *= 2) {
        W = H = s;
        atlas.assign(W * H, 0);
        int x = 0, y = 0, row = 0;
        std::vector<std::array<int, 8>> tmp;
        bool ok = true;
        for (int cp : cps) {
            auto *S = srcs.at(cp);
            float sc = stbtt_ScaleForPixelHeight(&S->font, (float)size);
            int w, h, ox, oy;
            unsigned char *bmp =
                stbtt_GetCodepointBitmap(&S->font, 0, sc, cp, &w, &h, &ox, &oy);
            if (x + w + PADDING > W) {
                x = 0;
                y += row + PADDING;
                row = 0;
            }
            if (y + h + PADDING > H) {
                ok = false;
                stbtt_FreeBitmap(bmp, nullptr);
                break;
            }
            if (bmp) {
                for (int j = 0; j < h; ++j)
                    for (int i = 0; i < w; ++i) {
                        int dy = flip ? (y + h - 1 - j) : (y + j);
                        atlas[dy * W + x + i] = bmp[j * w + i];
                    }
                stbtt_FreeBitmap(bmp, nullptr);
            }
            int adv, ls;
            stbtt_GetCodepointHMetrics(&S->font, cp, &adv, &ls);
            int adv_px = std::round(adv * sc);
            if (adv_px < w + 1)
                adv_px = w + 3;
            tmp.push_back({x, y, w, h, adv_px, ox, oy, cp});
            x += w + PADDING;
            row = std::max(row, h);
        }
        if (ok) {
            info = std::move(tmp);
            return true;
        }
    }
    return false;
}

std::vector<unsigned char> rle_zero(const std::vector<unsigned char> &data) {
    std::vector<unsigned char> out;
    for (size_t i = 0; i < data.size();) {
        if (data[i] == 0) {
            size_t run = 1;
            while (i + run < data.size() && data[i + run] == 0 && run < 255)
                ++run;
            out.push_back(0);
            out.push_back((unsigned char)run);
            i += run;
        } else {
            out.push_back(data[i++]);
        }
    }
    return out;
}

int main(int c, char **v) {
    try {
        int glyph_size = -1;
        bool flip = true;
        std::string outdir = "../../src/resources/", outfile = "fonts.hpp";
        std::vector<std::pair<std::string, std::string>> inputs;
        for (int i = 1; i < c; ++i) {
            std::string a = v[i];
            if (a.rfind("--size=", 0) == 0)
                glyph_size = std::stoi(a.substr(7));
            else if (a == "--no-flip")
                flip = false;
            else if (a.rfind("--out=", 0) == 0)
                outfile = a.substr(6);
            else {
                auto p = a.find_last_of('-');
                if (p == std::string::npos || p <= 2)
                    throw std::runtime_error("Invalid font argument: " + a);
                inputs.emplace_back(a.substr(2, p - 2), a.substr(p + 1));
            }
        }
        if (glyph_size < 1)
            throw std::runtime_error("Specify --size=N");

        std::vector<GlyphSource> storage;
        std::map<int, const GlyphSource *> srcs;
        load_fonts(inputs, storage, srcs);

        std::vector<int> cps;
        for (auto &[cp, _] : srcs)
            cps.push_back(cp);
        std::sort(cps.begin(), cps.end());

        std::vector<unsigned char> atlas;
        std::vector<std::array<int, 8>> info;
        int W, H;
        if (!pack_glyphs(cps, srcs, atlas, info, W, H, glyph_size, flip))
            throw std::runtime_error("Atlas overflow");

        std::vector<unsigned char> bits;
        bits.reserve(atlas.size() / 8 + 1);
        unsigned char cb = 0;
        int bp = 0;
        for (auto px : atlas) {
            if (px >= 128)
                cb |= (1 << (7 - bp));
            if (++bp == 8) {
                bits.push_back(cb);
                cb = 0;
                bp = 0;
            }
        }
        if (bp)
            bits.push_back(cb);
        auto zr = rle_zero(bits);

        std::vector<uint32_t> raw;
        raw.reserve(info.size() * 3);
        for (auto &e : info) {
            uint32_t xy = ((uint32_t)e[0] << 16) | ((uint32_t)e[1] & 0xFFFF);
            int32_t woad = ((e[2] & 0x3F) << 24) | ((e[3] & 0x3F) << 18) |
                           ((e[4] & 0x3F) << 12) | ((e[5] & 0x3F) << 6) |
                           ((e[6] & 0x3F));
            raw.push_back(xy);
            raw.push_back((uint32_t)woad);
            raw.push_back((uint32_t)e[7]);
        }

        size_t raw_b = raw.size() * 4;
        size_t bound = mz_compressBound(raw_b);
        std::vector<unsigned char> gz(bound);
        mz_ulong sz = bound;

        std::cout << "raw size: " << raw.size() << "\n";
        std::cout << "codepoints count: " << srcs.size() << "\n";

        mz_compress(gz.data(), &sz, (unsigned char *)raw.data(), raw_b);
        gz.resize(sz);

        fs::create_directories(fs::path(outdir));
        std::ofstream o(fs::path(outdir) / outfile);
        o << "#pragma once\n#include <stdint.h>\n#include "
             "\"../external/miniz.hpp\"\n"
             "#include \"../higui/platform.hpp\"\n\n";
        o << "constexpr int FONT_ATLAS_W = " << W << ";\n";
        o << "constexpr int FONT_ATLAS_H = " << H << ";\n";
        o << "constexpr unsigned FONT_COUNT = " << (raw.size() / 3) << ";\n\n";

        o << "constexpr unsigned char compressed_font_bitmap[] = {";
        for (size_t i = 0; i < zr.size(); ++i)
            o << int(zr[i]) << (i + 1 < zr.size() ? "," : "");
        o << "};\n\n";

        o << R"(inline void decompress_font_bitmap(uint8_t* out) noexcept {
    const auto* in = compressed_font_bitmap;
    size_t i = 0, N = sizeof(compressed_font_bitmap);
    while (i < N) {
        uint8_t b = in[i++];
        if (b == 0 && i < N) {
            uint8_t cnt = in[i++];
            for (int r = 0; r < cnt; ++r)
                for (int k = 0; k < 8; ++k)
                    *out++ = 0;
        } else {
            for (int k = 0; k < 8; ++k)
                *out++ = (b & (1 << (7 - k))) ? 255 : 0;
        }
    }
})" << "\n\n";

        o << "struct GlyphInfoPacked { uint32_t x_y; int32_t w_h_adv_ox_oy; "
             "uint32_t codepoint; };\n";
        o << "struct GlyphInfo { int x, y, w, h, advance, offset_x, offset_y, "
             "codepoint; };\n\n";

        o << "constexpr unsigned char compressed_font_glyphs[] = {";
        for (size_t i = 0; i < gz.size(); ++i)
            o << int(gz[i]) << (i + 1 < gz.size() ? "," : "");
        o << "};\n\n";

        o << R"(inline bool decompress_font_glyphs(GlyphInfo* out) noexcept {
    size_t byte_cnt = FONT_COUNT * sizeof(GlyphInfoPacked);
    void* mem = hi::alloc(byte_cnt);
    if (!mem) return false;
    mz_uncompress(static_cast<unsigned char *>(mem), &byte_cnt, compressed_font_glyphs, sizeof(compressed_font_glyphs));
    const GlyphInfoPacked* p = reinterpret_cast<const GlyphInfoPacked*>(mem);
    for (size_t i = 0; i < FONT_COUNT; ++i) {
        uint32_t xy = p[i].x_y, rs = (uint32_t)p[i].w_h_adv_ox_oy;
        out[i].x = xy >> 16;
        out[i].y = xy & 0xFFFF;
        out[i].w = (rs >> 24) & 0x3F;
        out[i].h = (rs >> 18) & 0x3F;
        out[i].advance = (rs >> 12) & 0x3F;
        out[i].offset_x = (rs >> 6) & 0x3F;
        out[i].offset_y = (rs >> 0) & 0x3F;
        out[i].codepoint = p[i].codepoint;
    }
    hi::free(mem, byte_cnt);
    return true;
})" << "\n\n";

        std::cout << "[OK] Generated " << outfile << " with "
                  << (raw.size() / 3) << " glyphs.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "[error] " << e.what() << "\n";
        return 1;
    }
}
