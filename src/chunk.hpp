#pragma once

#include "higui/higui_debug.hpp"
#include <assert.h>

namespace hi {
struct Block {
    unsigned short id;   // 16 bits
    unsigned char light; // 8 bits
    unsigned char faces; // 8 bits (6 faces + 2 reserved bits)

    inline constexpr Block() noexcept : id(0), light(0), faces(0) {}
    inline constexpr Block(unsigned short id_, unsigned char light_,
                           unsigned char faces_) noexcept
        : id(id_), light(light_), faces(faces_) {}
}; // struct Block

struct Chunk {
    Chunk() = delete;
    Chunk(const Chunk &) = delete;
    Chunk(Chunk &&) = delete;

    constexpr static unsigned WIDTH = 16;
    constexpr static unsigned HEIGHT = 16;
    constexpr static unsigned DEPTH = 16;

    constexpr static unsigned BLOCKS_PER_CHUNK = WIDTH * HEIGHT * DEPTH;

    inline static unsigned get_block_index(unsigned x, unsigned y,
                                           unsigned z) noexcept {
        assert(x < WIDTH && y < HEIGHT && z < DEPTH);
        return x + y * WIDTH + z * WIDTH * HEIGHT;
    }

    inline static void generate_chunk(unsigned chunk_index,
                                      Block *out) noexcept {
        const unsigned chunk_offset = chunk_index * BLOCKS_PER_CHUNK;

        for (unsigned z = 0; z < Chunk::DEPTH; ++z) {
            for (unsigned y = 0; y < Chunk::HEIGHT; ++y) {
                for (unsigned x = 0; x < Chunk::WIDTH; ++x) {
                    const unsigned index =
                        Chunk::get_block_index(x, y, z) + chunk_offset;
                    // out[index] = {1, 8, 0b11111111};
                    if (y == -1)
                        out[index] = {1, 8, 0b00100000};
                    else
                        out[index] = {1, 8, 0b00100000};
                }
            }
        }
    }
}; // struct Chunk
} // namespace hi