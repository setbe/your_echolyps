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

void compress_rgba(const std::vector<unsigned char> &input,
                   std::vector<unsigned> &output) {
    size_t i = 0, total = input.size();
    while (i < total) {
        if (input[i] == 0) {
            size_t run = 0;
            while (i + run < total && input[i + run] == 0)
                ++run;
            size_t rem = run;
            while (rem > 255) {
                output.push_back(0);
                output.push_back(255);
                rem -= 255;
            }
            output.push_back(0);
            output.push_back(static_cast<unsigned>(rem));
            i += run;
        } else {
            output.push_back(input[i++]);
        }
    }
}

void write_texturepack_hpp(const fs::path &output_path,
                           const std::vector<unsigned> &compressed,
                           unsigned width, unsigned height) {
    fs::create_directories(output_path.parent_path());
    std::ofstream out(output_path);
    if (!out) {
        std::cerr << "Error: Cannot open header file: " << output_path << "\n";
        return;
    }
    out << "#pragma once\n\n";
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
    out << "constexpr unsigned TEXTUREPACK_ATLAS_HEIGHT = " << height
        << ";\n\n";
    out << "constexpr unsigned char compressed_atlas[] = {\n    ";
    for (size_t i = 0; i < compressed.size(); ++i) {
        out << compressed[i] << ((i + 1) % 16 == 0 ? ",\n    " : ", ");
    }
    out << "\n};\n\n";
    out << "inline void decompress_atlas(unsigned char* out) noexcept {\n";
    out << "    unsigned i = 0, j = 0;\n";
    out << "    const unsigned total = sizeof(compressed_atlas) / "
           "sizeof(compressed_atlas[0]);\n";
    out << "    while (i < total) {\n";
    out << "        if (compressed_atlas[i] == 0) {\n";
    out << "            unsigned count = compressed_atlas[++i];\n";
    out << "            for (unsigned k = 0; k < count; ++k) out[j++] = 0;\n";
    out << "            ++i;\n";
    out << "        } else {\n";
    out << "            out[j++] = compressed_atlas[i++];\n";
    out << "        }\n";
    out << "    }\n";
    out << "}\n";
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

    fs::path output_hpp = current_dir / ".." / ".." / "src" / "texturepack.hpp";
    fs::path output_png = current_dir / ".." / ".." / "src" / "texturepack.png";
    fs::create_directories(output_hpp.parent_path());

    stbi_flip_vertically_on_write(flip_vertical);
    if (!stbi_write_png(output_png.string().c_str(), atlas_w, atlas_h, 4,
                        atlas.data(), atlas_w * 4)) {
        std::cerr << "Error: Failed to write PNG: " << output_png << "\n";
        return 1;
    }
    std::cout << "PNG atlas written to: " << output_png << "\n";

    std::vector<unsigned> compressed;
    compress_rgba(atlas, compressed);
    write_texturepack_hpp(output_hpp, compressed, atlas_w, atlas_h);
    std::cout << "Header written to: " << output_hpp << "\n";

    return 0;
}
