#pragma once

#include "../external/PerlinNoise.hpp"
#include "../external/linmath.hpp"
#include "block.hpp"

#include <assert.h>
#include <vector> // for std::hash

namespace hi {
struct NoiseSystem {
    siv::PerlinNoise height;
}; // struct NoiseSystem
} // namespace hi

namespace hi::Chunk {
struct Mesh {
    unsigned vertex_offset;
    unsigned vertex_count;
    float world_x, world_y, world_z;
}; // struct Mesh
struct Key {
    int x, y, z;
    bool operator==(const Key &o) const noexcept {
        return x == o.x && y == o.y && z == o.z;
    }
    struct Hash {
        size_t operator()(const Key &k) const noexcept {
            size_t h1 = std::hash<int>{}(k.x);
            size_t h2 = std::hash<int>{}(k.y);
            size_t h3 = std::hash<int>{}(k.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    }; // struct Key::Hash
}; // struct Key

constexpr unsigned WIDTH = 32;
constexpr unsigned HEIGHT = 32;
constexpr unsigned DEPTH = 32;

constexpr unsigned BLOCKS_PER_CHUNK = WIDTH * HEIGHT * DEPTH;

inline unsigned calculate_block_index(unsigned x, unsigned y,
                                      unsigned z) noexcept {
    assert(x < WIDTH && y < HEIGHT && z < DEPTH);
    return x + y * WIDTH + z * WIDTH * HEIGHT;
} // calculate_block_index

void generate_chunk(unsigned cx, unsigned cy, unsigned cz, Block *out,
                    const NoiseSystem &noise, unsigned lod_size = 1) noexcept;

void generate_block(int gx, int gy, int gz, unsigned idx, Block *out,
                    const NoiseSystem &noise) noexcept;

inline bool is_block_on_chunk_edge(int x, int y, int z) noexcept {
    return x == 0 || x == Chunk::WIDTH - 1 || y == 0 ||
           y == Chunk::HEIGHT - 1 || z == 0 || z == Chunk::DEPTH - 1;
} // is_block_on_chunk_edge

inline bool is_chunk_visible(const Mesh &mesh,
                             const float frustum_planes[6][4]) noexcept {
    const float x0 = mesh.world_x;
    const float y0 = mesh.world_y;
    const float z0 = mesh.world_z;
    const float x1 = x0 + Chunk::WIDTH;
    const float y1 = y0 + Chunk::HEIGHT;
    const float z1 = z0 + Chunk::DEPTH;

    for (int i = 0; i < 6; ++i) {
        const float *p = frustum_planes[i];
        const float vx = (p[0] > 0.0f) ? x1 : x0;
        const float vy = (p[1] > 0.0f) ? y1 : y0;
        const float vz = (p[2] > 0.0f) ? z1 : z0;
        if (p[0] * vx + p[1] * vy + p[2] * vz + p[3] < 0.0f)
            return false;
    }
    return true;
} // is_chunk_visible

inline void extract_frustum_planes(float planes[6][4],
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
} // extract_frustum_planes
} // namespace hi::Chunk