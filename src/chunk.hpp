#pragma once

#include "external/PerlinNoise.hpp"
#include "external/linmath.hpp"
#include "higui/higui_debug.hpp"
#include <assert.h>
#include <string.h>

namespace hi {
struct Block {
    uint16_t id;    // 16 bits
    uint16_t flags; // 16 bits: [0..3]=light, [4..9]=faces, [10..15]=reserved

    inline constexpr Block() noexcept : id(0), flags(0) {}

    inline constexpr Block(uint16_t id_, uint8_t light, uint8_t faces) noexcept
        : id(id_), flags((light & 0xF) | ((faces & 0x3F) << 4)) {}

    inline uint8_t get_light() const noexcept { return flags & 0xF; }

    inline uint8_t get_faces() const noexcept { return (flags >> 4) & 0x3F; }

    inline void set_light(uint8_t light) noexcept {
        flags = (flags & ~0xF) | (light & 0xF);
    }

    inline void set_faces(uint8_t faces) noexcept {
        flags = (flags & ~(0x3F << 4)) | ((faces & 0x3F) << 4);
    }

    // Packing Block to float
    inline float to_float() const noexcept {
        uint32_t packed = ((uint32_t)flags << 16) | id;
        float result;
        memcpy(&result, &packed, sizeof(float));
        return result;
    }

    // Unpacking Block from float
    static inline Block from_float(float f) noexcept {
        uint32_t packed;
        memcpy(&packed, &f, sizeof(uint32_t));
        Block b;
        b.id = packed & 0xFFFF;
        b.flags = packed >> 16;
        return b;
    }
};

// Do not create `Chunk` instances.
// This struct will be refactored into namespace
// (if everything will go according to the plan)
struct Chunk {
    struct Mesh {
        unsigned vertex_offset;
        unsigned vertex_count;
        float world_x, world_y, world_z;
    };
    Chunk() = delete; // delete constructor
    Chunk(const Chunk &) = delete;
    Chunk(Chunk &&) = delete;

    constexpr static unsigned WIDTH = 16;
    constexpr static unsigned HEIGHT = 16;
    constexpr static unsigned DEPTH = 16;

    constexpr static unsigned BLOCKS_PER_CHUNK = WIDTH * HEIGHT * DEPTH;

    inline static unsigned calculate_block_index(unsigned x, unsigned y,
                                                 unsigned z) noexcept {
        assert(x < WIDTH && y < HEIGHT && z < DEPTH);
        return x + y * WIDTH + z * WIDTH * HEIGHT;
    }

    inline static void generate_chunk(unsigned chunk_x, unsigned chunk_y,
                                      unsigned chunk_z, Block *out,
                                      const siv::PerlinNoise &noise,
                                      unsigned lod_size = 1) noexcept {
        for (unsigned z = 0; z < DEPTH; z += lod_size)
            for (unsigned y = 0; y < HEIGHT; y += lod_size)
                for (unsigned x = 0; x < WIDTH; x += lod_size) {
                    const unsigned index = calculate_block_index(x, y, z);
                    const unsigned gx = chunk_x * WIDTH + x;
                    const unsigned gy = chunk_y * HEIGHT + y;
                    const unsigned gz = chunk_z * DEPTH + z;

                    const double h =
                        noise.octave2D_01(gx * 0.01, gz * 0.01, 4) * 32.0;

                    if (double(gy) < h) {
                        out[index] = {1, 8, 0b111111};
                    } else {
                        out[index] = {0, 0, 0};
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