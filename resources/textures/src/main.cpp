#include "../../../src/external/miniz.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static const std::vector<std::string> blocks = {
    "grass1.png", "grass2.png", "grass3.png", "cobblestone.png", "t1.png",
    "t2.png",     "t3.png",     "t4.png",     "t5.png",          "t6.png"};
static const std::vector<std::string> items = {
    // empty for now
};

bool flip_vertical = true;

struct Image {
    int width;
    int height;
    std::vector<unsigned char> data;
};

Image load_or_empty(const fs::path &filepath, int tile_w, int tile_h) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(flip_vertical);
    unsigned char *pixels =
        stbi_load(filepath.string().c_str(), &w, &h, &channels, 4);
    if (pixels && w == tile_w && h == tile_h) {
        std::vector<unsigned char> data(pixels, pixels + w * h * 4);
        stbi_image_free(pixels);
        return {w, h, std::move(data)};
    }
    if (pixels)
        stbi_image_free(pixels);
    std::cerr << "Warning: Missing or invalid texture: " << filepath << "\n";
    return {tile_w, tile_h, std::vector<unsigned char>(tile_w * tile_h * 4, 0)};
}

unsigned nextPowerOfTwo(unsigned n) {
    unsigned p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

std::vector<unsigned char>
compress_with_miniz(const std::vector<unsigned char> &input) {
    size_t bound = mz_compressBound(input.size());
    std::vector<unsigned char> out(bound);
    mz_ulong out_len = bound;
    if (mz_compress(out.data(), &out_len, input.data(), input.size()) != Z_OK)
        throw std::runtime_error("miniz compression failed");
    out.resize(out_len);
    return out;
}

void write_texturepack_hpp(const fs::path &output_path,
                           const std::vector<unsigned char> &compressed,
                           unsigned width, unsigned height) {
    fs::create_directories(output_path.parent_path());
    std::ofstream out(output_path);
    if (!out) {
        std::cerr << "Error: Cannot open header file: " << output_path << "\n";
        return;
    }

    out << "#pragma once\n";
    out << "#include <stdint.h>\n";
    out << "#include \"../external/miniz.hpp\"\n";
    out << "#include \"../higui/platform.hpp\"\n\n";

    out << "enum class Texturepack : unsigned short {\n";
    for (size_t i = 0; i < blocks.size(); ++i) {
        auto name = blocks[i];
        auto dot = name.find_last_of('.');
        out << "    " << name.substr(0, dot);
        if (i + 1 < blocks.size())
            out << "= " << (i + 1) << ",";
        out << "\n";
    }
    out << "};\n\n";

    out << "constexpr unsigned TEXTUREPACK_ATLAS_WIDTH = " << width << ";\n";
    out << "constexpr unsigned TEXTUREPACK_ATLAS_HEIGHT = " << height << ";\n";
    out << "constexpr size_t TEXTUREPACK_DECOMPRESSED_SIZE = "
           "TEXTUREPACK_ATLAS_WIDTH * "
           "TEXTUREPACK_ATLAS_HEIGHT * 4;\n\n";

    out << "constexpr unsigned char compressed_atlas[] = {";
    for (size_t i = 0; i < compressed.size(); ++i)
        out << (int)compressed[i] << (i + 1 < compressed.size() ? "," : "");
    out << "};\n\n";

    out << R"(inline bool decompress_atlas(uint8_t* out) noexcept {
    size_t sz = TEXTUREPACK_DECOMPRESSED_SIZE;
    return mz_uncompress(out, &sz, compressed_atlas, sizeof(compressed_atlas)) == Z_OK;
})" << "\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: packer <texturepack_name>\n";
        return 1;
    }
    fs::path current_dir = fs::current_path();
    fs::path texturepack_dir = current_dir / "texturepacks" / argv[1];
    if (!fs::exists(texturepack_dir)) {
        std::cerr << "Error: Texturepack not found: " << texturepack_dir
                  << "\n";
        return 1;
    }

    const int tile_w = 16, tile_h = 16;
    size_t total_tiles = blocks.size() + items.size();
    int per_row = static_cast<int>(
        std::ceil(std::sqrt(static_cast<double>(total_tiles))));
    unsigned atlas_size = nextPowerOfTwo(per_row * tile_w);
    unsigned atlas_w = atlas_size, atlas_h = atlas_size;
    std::vector<unsigned char> atlas(atlas_w * atlas_h * 4, 0);

    auto blit = [&](const Image &img, size_t idx) {
        int row = static_cast<int>(idx) / per_row;
        int col = static_cast<int>(idx) % per_row;
        for (int y = 0; y < tile_h; ++y) {
            for (int x = 0; x < tile_w; ++x) {
                for (int ch = 0; ch < 4; ++ch) {
                    size_t dst =
                        ((row * tile_h + y) * atlas_w + (col * tile_w + x)) *
                            4 +
                        ch;
                    size_t src = (y * tile_w + x) * 4 + ch;
                    atlas[dst] = img.data[src];
                }
            }
        }
    };
    for (size_t i = 0; i < blocks.size(); ++i) {
        blit(load_or_empty(texturepack_dir / "blocks" / blocks[i], tile_w,
                           tile_h),
             i);
    }
    for (size_t i = 0; i < items.size(); ++i) {
        blit(
            load_or_empty(texturepack_dir / "items" / items[i], tile_w, tile_h),
            blocks.size() + i);
    }

    fs::path output_hpp =
        current_dir / ".." / ".." / "src" / "resources" / "texturepack.hpp";
    fs::path output_png =
        current_dir / ".." / ".." / "src" / "resources" / "texturepack.png";

    stbi_flip_vertically_on_write(flip_vertical);
    if (!stbi_write_png(output_png.string().c_str(), atlas_w, atlas_h, 4,
                        atlas.data(), atlas_w * 4)) {
        std::cerr << "Error: Failed to write PNG: " << output_png << "\n";
        return 1;
    }
    std::cout << "PNG atlas written to: " << output_png << "\n";

    std::vector<unsigned char> compressed = compress_with_miniz(atlas);
    write_texturepack_hpp(output_hpp, compressed, atlas_w, atlas_h);
    std::cout << "Header written to: " << output_hpp << "\n";

    return 0;
}
