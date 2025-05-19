#include "chunk.hpp"
#include "block_list.hpp"
#include <algorithm>
#include <complex>

namespace hi::Chunk {
void generate_chunk(unsigned cx, unsigned cy, unsigned cz, Block *out,
                    const NoiseSystem &noise, unsigned lod_size) noexcept {
    if (cy > 3)
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

void generate_block(int gx, int gy, int gz, unsigned idx, Block *out,
                    const NoiseSystem &noise) noexcept {
    constexpr int SEA_LEVEL = 40;
    constexpr int DIRT_DEPTH = 4;
    constexpr double scale = 0.004;

    double h_noise = noise.height.octave2D_01(gx * scale, gz * scale, 4);
    int H = int(h_noise * 90.0); // 0–90 meters height

    // === Генерація блоків ===
    using namespace BlockList;
    if (gy > H) {
        out[idx] = (gy <= SEA_LEVEL) ? Water : Air;
    } else if (gy == H) {
        out[idx] = Grass;
    } else if (gy > H - DIRT_DEPTH) {
        out[idx] = Dirt;
    } else {
        out[idx] = Cobblestone;
    }
}

} // namespace hi::Chunk
