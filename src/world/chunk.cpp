#include "chunk.hpp"
#include "block_list.hpp"
#include <algorithm>
#include <complex>

constexpr unsigned MAX_HEIGHT_CHUNKS = 4;

namespace hi::Chunk {
void generate_chunk(unsigned cx, unsigned cy, unsigned cz, Block *out,
                    const NoiseSystem &noise, unsigned lod_size) noexcept {
    if (cy > MAX_HEIGHT_CHUNKS)
        return;
    for (unsigned z = 0; z < DEPTH; z += lod_size)
        for (unsigned y = 0; y < HEIGHT; y += lod_size)
            for (unsigned x = 0; x < WIDTH; x += lod_size) {
                unsigned idx = calculate_block_index(x, y, z);
                int gx = int(cx * WIDTH + x);
                int gy = int(cy * HEIGHT + y);
                int gz = int(cz * DEPTH + z);
                generate_block(gx, gy, gz, idx, out, noise);
            }
} // generate_chunk

inline constexpr void apply_simple_light_to_block(Block &block, int middle,
                                                  int gy) {
    constexpr int threshold = 40; // gradient size
    int delta = std::abs(gy - middle);

    uint8_t light;
    if (delta >= threshold) {
        light = 0;
    } else {
        float factor = 1.0f - float(delta) / float(threshold);    // [0..1]
        light = static_cast<uint8_t>(std::round(factor * 15.0f)); // [0..15]
    }

    block.set_light(light);
}

void generate_block(int gx, int gy, int gz, unsigned idx, Block *out,
                    const NoiseSystem &noise) noexcept {
    constexpr int SEA_LEVEL = 40;
    constexpr int BEACH_LEVEL = SEA_LEVEL + 1;
    constexpr int MOUNTAIN_ICE_LEVEL = MAX_HEIGHT_CHUNKS * Chunk::HEIGHT - 4;
    constexpr int TERRAIN_MIDDLE_LEVEL = MOUNTAIN_ICE_LEVEL - SEA_LEVEL;

    constexpr int DIRT_DEPTH = 4;

    constexpr double scale = 0.004;

    // === Noise Manipulation ===
    double h1 = noise.height.octave2D_01(
        /* x    */ gx * scale,
        /* y    */ gz * scale,
        /* oct  */ 4,
        /* pers */ 0.4);
    double h2 = noise.height.octave2D_01(
        /* x    */ gx * scale * 1.3,
        /* y    */ gz * scale * 1.3,
        /* oct  */ 2,
        /* pers */ 0.3);
    double h = std::pow(h1 + h2 * 0.3, 2.0);

    int H = int(h * MAX_HEIGHT_CHUNKS * Chunk::HEIGHT);

    // === Placement Logic ===
    using namespace BlockList;
    if (gy > H) {
        out[idx] = (gy <= SEA_LEVEL) ? Water : Air;
    } else if (gy <= SEA_LEVEL) {
        out[idx] = Water;
    } else if (gy == H && gy <= BEACH_LEVEL) {
        out[idx] = Sand;
    } else if (gy == H) {
        out[idx] = (gy <= MOUNTAIN_ICE_LEVEL) ? Grass : Ice;
    } else if (gy > H - DIRT_DEPTH) {
        out[idx] = Dirt;
    } else {
        out[idx] = Cobblestone;
    }

    apply_simple_light_to_block(out[idx], TERRAIN_MIDDLE_LEVEL, gy);
}

} // namespace hi::Chunk
