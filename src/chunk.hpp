#pragma once

#include "external/PerlinNoise.hpp"
#include "external/linmath.hpp"
#include "higui/higui_debug.hpp"
#include "texturepack.hpp"
#include <assert.h>
#include <string.h>

namespace hi {
struct Block {
    // 12 bits - actual id, 4 bits - texture layout
    // 0000'0000'0000|0000 actual id|texture layout
    uint16_t id;

    // 4 bits - light, 6 - visible faces, 1 - is transparent
    // 5 - reserved
    // 0000|0000'00|0 light|faces|is_transparent
    uint16_t flags;

    // Constructors
    inline constexpr Block(uint16_t id, uint8_t light, uint8_t faces,
                           bool transparent = false) noexcept
        : id{id}, flags{make_flags(light, faces, transparent)} {}

    inline constexpr Block(uint16_t id, uint16_t flags) noexcept
        : id{id}, flags{flags} {}

    template <uint16_t T_BlockID, uint16_t T_TextureMask>
    static constexpr uint16_t make_id() noexcept {
        return static_cast<uint16_t>((T_BlockID & 0x0FFF) |
                                     ((T_TextureMask & 0x000F) << 12));
    }

    static constexpr uint16_t make_flags(uint8_t light, uint8_t faces,
                                         bool transparent) noexcept {
        return static_cast<uint16_t>((light & 0x0F) | ((faces & 0x3F) << 4) |
                                     (transparent ? (1 << 10) : 0));
    }

    // Accessors
    inline uint16_t block_id() const noexcept { return id & 0x0FFF; }
    inline uint16_t texture_protocol() const noexcept {
        return (id >> 12) & 0x000F;
    }
    inline uint8_t light() const noexcept { return flags & 0x000F; }
    inline uint8_t faces() const noexcept { return (flags >> 4) & 0x003F; }
    inline bool transparent() const noexcept {
        return ((flags >> 10) & 0x0001) != 0;
    }

    inline void set_block_id(uint16_t block_id) noexcept {
        id = static_cast<uint16_t>((id & 0xF000) | (block_id & 0x0FFF));
    }
    inline void set_texture_protocol(uint16_t proto) noexcept {
        id = static_cast<uint16_t>((id & 0x0FFF) | ((proto & 0x000F) << 12));
    }
    inline void set_light(uint8_t l) noexcept {
        flags = static_cast<uint16_t>((flags & ~0x000F) | (l & 0x0F));
    }
    inline void set_faces(uint8_t f) noexcept {
        flags = static_cast<uint16_t>((flags & ~0x03F0) | ((f & 0x3F) << 4));
    }
    inline void set_transparent(bool t) noexcept {
        flags = t ? static_cast<uint16_t>(flags | (1 << 10))
                  : static_cast<uint16_t>(flags & ~(1 << 10));
    }

    // Packing to float
    inline float to_float() const noexcept {
        uint32_t packed = (static_cast<uint32_t>(flags) << 16) | id;
        float result;
        memcpy(&result, &packed, sizeof(result));
        return result;
    }
    static inline Block from_float(float f) noexcept {
        uint32_t packed;
        memcpy(&packed, &f, sizeof(packed));
        return {static_cast<uint16_t>(packed & 0xFFFF),
                static_cast<uint16_t>(packed >> 16)};
    }
}; // struct Block

struct BlockList {
    struct Texture {
        constexpr static uint16_t RESOLUTION = 16;
        // Texture masks for ID high bits
        constexpr static uint16_t ONE = 0b1000;
        constexpr static uint16_t TWO =
            0b0100; // 1st for top face, 2nd for others faces
        constexpr static uint16_t THREE_FRONT =
            0b0010; // 1st top, 2nd front, 3rd others
        constexpr static uint16_t THREE_SIDES =
            0b0001; // 1st top; 2nd front, back, left, right; 3rd bottom
        constexpr static uint16_t SIX = 0b0000; // unique textures per face
    }; // struct Texture

#define tex(name) (static_cast<uint16_t>(Texturepack::name))
#define block(id, texture_mask)                                                \
    {Block::make_id<id, texture_mask>(), 8, 0b1111'1100, false}
#define var constexpr static Block
    // BLOCK LIST BEGIN
    var Air = block(0, Texture::ONE);
    var Grass = block(tex(grass1), Texture::THREE_SIDES);
    var Dirt = block(tex(grass3), Texture::ONE);
    var Cobblestone = block(tex(cobblestone), Texture::ONE);
    var Tblock = block(tex(t1), Texture::SIX);
    // BLOCK LIST END
#undef var
#undef block
}; // struct BlockList

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

    inline static void generate_chunk(unsigned cx, unsigned cy, unsigned cz,
                                      Block *out, const siv::PerlinNoise &noise,
                                      unsigned lod_size = 1) noexcept {
        constexpr int DIRT_DEPTH = 4;
        for (unsigned z = 0; z < DEPTH; z += lod_size)
            for (unsigned y = 0; y < HEIGHT; y += lod_size)
                for (unsigned x = 0; x < WIDTH; x += lod_size) {
                    unsigned idx = calculate_block_index(x, y, z);
                    int gx = int(cx * WIDTH + x);
                    int gy = int(cy * HEIGHT + y);
                    int gz = int(cz * DEPTH + z);

                    double h =
                        noise.octave2D_01(gx * 0.01, gz * 0.01, 4) * 32.0;
                    int H = int(math::floorf(static_cast<float>(h)));

                    if (gy > H) {
                        out[idx] = BlockList::Air;
                    } else if (gy == H) {
                        out[idx] = BlockList::Grass;
                    } else if (gy > H - DIRT_DEPTH) {
                        out[idx] = BlockList::Dirt;
                    } else {
                        out[idx] = BlockList::Cobblestone;
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