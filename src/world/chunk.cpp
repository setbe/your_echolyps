#include "chunk.hpp"
#include "block_list.hpp"
#include <algorithm>
#include <complex>

namespace hi::Chunk {
void generate_chunk(unsigned cx, unsigned cy, unsigned cz, Block *out,
                    const NoiseSystem &noise, unsigned lod_size) noexcept {
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

inline static Block get_grass(BiomeType biome) noexcept;
inline static Block get_dirt(BiomeType biome) noexcept;

inline double smoothstep(double edge0, double edge1, double x) noexcept {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

// Generates a single block at global coordinates (gx, gy, gz)
void generate_block(int gx, int gy, int gz, unsigned idx, Block *out,
                    const NoiseSystem &noise) noexcept {
    constexpr int DIRT_DEPTH = 4;
    constexpr double mod = 0.008;

    // === Біомні карти ===
    double temp = noise.temp.noise2D_01(gx * mod, gz * mod);
    double humid = noise.humid.noise2D_01(gx * mod, gz * mod);

    // --- Плавні ваги біомів через smoothstep ---
    double plains_w = 1.0 - smoothstep(0.2, 0.8, std::abs(temp - 0.5)); // центр
    double forest_w =
        1.0 - smoothstep(0.4, 0.9, std::abs(humid - 0.7)); // вологі
    double snow_w = 1.0 - smoothstep(0.0, 0.6, temp);      // холодні
    double savannah_w = smoothstep(0.6, 0.85, temp) *
                        (1.0 - smoothstep(0.5, 0.7, humid)); // гарячі + сухі

    // Нормалізація
    double total = plains_w + forest_w + snow_w + savannah_w + 1e-6; // захист
    plains_w /= total;
    forest_w /= total;
    snow_w /= total;
    savannah_w /= total;

    // --- Параметри від біомів ---
    double amp =
        plains_w * 8.0 + forest_w * 16.0 + snow_w * 32.0 + savannah_w * 5.0;
    double freq = plains_w * 0.008 + forest_w * 0.01 + snow_w * 0.006 +
                  savannah_w * 0.012;

    // --- Остаточний біом для вигляду блока (трава/земля) ---
    BiomeType biome = BiomeType::Plains;
    double max_w = plains_w;
    if (forest_w > max_w) {
        biome = BiomeType::Forest;
        max_w = forest_w;
    }
    if (snow_w > max_w) {
        biome = BiomeType::Snow;
        max_w = snow_w;
    }
    if (savannah_w > max_w) {
        biome = BiomeType::Savannah;
    }

    // --- Генерація висоти ---
    double global = noise.continent.octave2D_01(gx * 0.0015, gz * 0.0015,
                                                5); // більші масиви
    double local = noise.height.octave2D_01(gx * freq, gz * freq, 4);
    double shape = global * 0.85 + local * 0.15;

    // Висота з глибшими змінами
    int H = int(std::pow(shape, 1.35) * amp * 12.5);

    // --- Призначення блоку ---
    if (gy > H) {
        out[idx] = BlockList::Air;
    } else if (gy == H) {
        out[idx] = get_grass(biome);
    } else if (gy > H - DIRT_DEPTH) {
        out[idx] = get_dirt(biome);
    } else {
        out[idx] = BlockList::Cobblestone;
    }
}

// Returns the appropriate grass block type based on biome
inline static Block get_grass(BiomeType biome) noexcept {
    // clang-format off
    switch (biome) {
    case BiomeType::Plains:   return BlockList::Grass_Plains;
    case BiomeType::Savannah: return BlockList::Grass_Savannah;
    case BiomeType::Forest:   return BlockList::Grass_Forest;
    case BiomeType::Snow:     return BlockList::Grass_Snow;

    default:                  return BlockList::Grass_Forest;
    }
    // clang-format on
}

// Returns the appropriate dirt block type based on biome
inline static Block get_dirt(BiomeType biome) noexcept {
    // clang-format off
    switch (biome) {
    case BiomeType::Plains:   return BlockList::Dirt_Plains;
    case BiomeType::Savannah: return BlockList::Dirt_Savannah;
    case BiomeType::Forest:   return BlockList::Dirt_Forest;
    case BiomeType::Snow:     return BlockList::Dirt_Snow;

    default:                  return BlockList::Dirt_Forest;
    }
    // clang-format on
}

} // namespace hi::Chunk
