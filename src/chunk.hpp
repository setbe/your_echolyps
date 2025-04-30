#pragma once

#include "external/linmath.hpp"
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
    struct Mesh {
        uint32_t vertex_offset;
        uint32_t vertex_count;
        float world_x, world_y, world_z;
    };
    Chunk() = delete;
    Chunk(const Chunk &) = delete;
    Chunk(Chunk &&) = delete;

    constexpr static unsigned WIDTH = 32;
    constexpr static unsigned HEIGHT = 32;
    constexpr static unsigned DEPTH = 32;

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

    inline static bool is_block_on_chunk_edge(int x, int y, int z) noexcept {
        return x == 0 || x == Chunk::WIDTH - 1 || y == 0 ||
               y == Chunk::HEIGHT - 1 || z == 0 || z == Chunk::DEPTH - 1;
    }

    inline static bool
    is_chunk_visible(const Mesh &mesh,
                     const float frustum_planes[6][4]) noexcept {
        const float x0 = mesh.world_x;
        const float y0 = mesh.world_y;
        const float z0 = mesh.world_z;
        const float x1 = x0 + Chunk::WIDTH;
        const float y1 = y0 + Chunk::HEIGHT;
        const float z1 = z0 + Chunk::DEPTH;

        for (int i = 0; i < 6; ++i) {
            const float *p = frustum_planes[i];
            // closest to outside
            float vx = (p[0] < 0) ? x0 : x1;
            float vy = (p[1] < 0) ? y0 : y1;
            float vz = (p[2] < 0) ? z0 : z1;
            if (p[0] * vx + p[1] * vy + p[2] * vz + p[3] < 0)
                return false;
        }
        return true;
    }

    inline static void
    extract_frustum_planes(float planes[6][4],
                           const math::mat4x4 view) noexcept {
        const float *m = &view[0][0];
        // left
        planes[0][0] = m[3] + m[0];
        planes[0][1] = m[7] + m[4];
        planes[0][2] = m[11] + m[8];
        planes[0][3] = m[15] + m[12];
        // right
        planes[1][0] = m[3] - m[0];
        planes[1][1] = m[7] - m[4];
        planes[1][2] = m[11] - m[8];
        planes[1][3] = m[15] - m[12];
        // bottom
        planes[2][0] = m[3] + m[1];
        planes[2][1] = m[7] + m[5];
        planes[2][2] = m[11] + m[9];
        planes[2][3] = m[15] + m[13];
        // top
        planes[3][0] = m[3] - m[1];
        planes[3][1] = m[7] - m[5];
        planes[3][2] = m[11] - m[9];
        planes[3][3] = m[15] - m[13];
        // near
        planes[4][0] = m[3] + m[2];
        planes[4][1] = m[7] + m[6];
        planes[4][2] = m[11] + m[10];
        planes[4][3] = m[15] + m[14];
        // far
        planes[5][0] = m[3] - m[2];
        planes[5][1] = m[7] - m[6];
        planes[5][2] = m[11] - m[10];
        planes[5][3] = m[15] - m[14];
    }

}; // struct Chunk
} // namespace hi