#pragma once

#include <stdint.h>
#include <string.h>

namespace hi {
struct Block {
    struct TextureProtocol {
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

        static constexpr uint32_t resolve_offset(uint16_t protocol,
                                                 int face) noexcept {
            switch (protocol) {
            case ONE:
                return 0;
            case TWO:
                return (face == 4) ? 0 : 1; // top = 0, others = 1
            case THREE_FRONT:
                return (face == 4)
                           ? 0
                           : (face == 0 ? 1
                                        : 2); // top = 0, front = 1, others = 2
            case THREE_SIDES:
                return (face == 4)
                           ? 0
                           : (face == 5 ? 2
                                        : 1); // top = 0, bottom = 2, sides = 1
            case SIX:
            default:
                return face; // unique texture per face
            }
        }
    }; // struct TextureProtocol

    /* 12 bits - actual id, 4 bits - texture layout

       0000'0000'0000 | 0000      actual id | texture layout

   */
    uint16_t id;

    /* 4 bits - light, 6 - visible faces, 1 - is transparent
       5 - reserved

       0000 | 0000'00 | 0        light | faces | transparent

   */
    uint16_t flags;

    // 0000'0000'0000 | 0000 <=> actual id | texture layout
    // 0000 | 0000'00 | 0 <=> light | faces | transparent
    inline constexpr Block(uint16_t id, uint8_t light, uint8_t faces,
                           bool transparent = false) noexcept
        : id{id}, flags{make_flags(light, faces, transparent)} {}

    // 0000'0000'0000 | 0000 <=> actual id | texture layout
    inline constexpr Block(uint16_t id = 0, uint16_t flags = 0) noexcept
        : id{id}, flags{flags} {}

    // 0000'0000'0000 | 0000 <=> actual id | texture layout
    static constexpr uint16_t make_id(uint16_t raw_id,
                                      uint16_t proto) noexcept {
        return static_cast<uint16_t>((raw_id & 0x0FFF) |
                                     ((proto & 0x000F) << 12));
    }

    // 0000 | 0000'00 | 0  <=> light | faces | transparent
    static constexpr uint16_t make_flags(uint8_t light, uint8_t faces,
                                         bool transparent) noexcept {
        return static_cast<uint16_t>(((light & 0x0F) << 12) |
                                     ((faces & 0x3F) << 6) |
                                     (transparent ? (1 << 5) : 0));
    }

    // Accessors
    inline constexpr uint16_t block_id() const noexcept { return id & 0x0FFF; }
    inline constexpr uint16_t texture_protocol() const noexcept {
        return (id >> 12) & 0x000F;
    }
    // 0000 | 0000'00 | 0 <=> light | faces | transparent
    inline constexpr uint8_t light() const noexcept {
        return (flags >> 12) & 0x0F;
    }

    // 0000 | 0000'00 | 0 <=> light | faces | transparent
    inline constexpr uint8_t faces() const noexcept {
        return (flags >> 4) & 0x003F;
    }

    // 0000 | 0000'00 | 0 <=> light | faces | transparent
    inline constexpr bool transparent() const noexcept {
        return ((flags >> 10) & 0x0001) != 0;
    }

    // 0000'0000'0000 | 0000 <=> actual id | texture layout
    inline void set_block_id(uint16_t block_id) noexcept {
        id = static_cast<uint16_t>((id & 0xF000) | (block_id & 0x0FFF));
    }

    // 0000'0000'0000 | 0000 <=> actual id | texture layout
    inline void set_texture_protocol(uint16_t proto) noexcept {
        id = static_cast<uint16_t>((id & 0x0FFF) | ((proto & 0x000F) << 12));
    }

    // 0000 | 0000'00 | 0 <=> light | faces | transparent
    inline constexpr void set_light(uint8_t light) noexcept {
        flags =
            static_cast<uint16_t>((flags & 0x0FFF) | ((light & 0x0F) << 12));
    }

    // 0000 | 0000'00 | 0 <=> light | faces | transparent
    inline void set_faces(uint8_t f) noexcept {
        flags = static_cast<uint16_t>((flags & ~0x03F0) | ((f & 0x3F) << 4));
    }

    // 0000 | 0000'00 | 0 <=> light | faces | transparent
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

    constexpr static float CUBE_POS[6][18] = {
        // front (z+)
        {/* BL(0,0,1) */ 0.f, 0.f, 1.f, /* BR(1,0,1) */ 1.f, 0.f, 1.f,
         /* TR(1,1,1) */ 1.f, 1.f, 1.f, /* BL        */ 0.f, 0.f, 1.f,
         /* TR        */ 1.f, 1.f, 1.f, /* TL(0,1,1) */ 0.f, 1.f, 1.f},

        // back (z-)
        {/* BL(1,0,0) */ 1.f, 0.f, 0.f, /* BR(0,0,0) */ 0.f, 0.f, 0.f,
         /* TR(0,1,0) */ 0.f, 1.f, 0.f, /* BL        */ 1.f, 0.f, 0.f,
         /* TR        */ 0.f, 1.f, 0.f, /* TL(1,1,0) */ 1.f, 1.f, 0.f},

        // left (x-)
        {/* BL(0,0,0) */ 0.f, 0.f, 0.f, /* BR(0,0,1) */ 0.f, 0.f, 1.f,
         /* TR(0,1,1) */ 0.f, 1.f, 1.f, /* BL        */ 0.f, 0.f, 0.f,
         /* TR        */ 0.f, 1.f, 1.f, /* TL(0,1,0) */ 0.f, 1.f, 0.f},

        // right (x+)
        {/* BL(1,0,1) */ 1.f, 0.f, 1.f, /* BR(1,0,0) */ 1.f, 0.f, 0.f,
         /* TR(1,1,0) */ 1.f, 1.f, 0.f, /* BL        */ 1.f, 0.f, 1.f,
         /* TR        */ 1.f, 1.f, 0.f, /* TL(1,1,1) */ 1.f, 1.f, 1.f},

        // top (y+)
        {/* BL(0,1,1) */ 0.f, 1.f, 1.f, /* BR(1,1,1) */ 1.f, 1.f, 1.f,
         /* TR(1,1,0) */ 1.f, 1.f, 0.f, /* BL        */ 0.f, 1.f, 1.f,
         /* TR        */ 1.f, 1.f, 0.f, /* TL(0,1,0) */ 0.f, 1.f, 0.f},

        // bottom (y-)
        {/* BL(0,0,0) */ 0.f, 0.f, 0.f, /* BR(1,0,0) */ 1.f, 0.f, 0.f,
         /* TR(1,0,1) */ 1.f, 0.f, 1.f, /* BL        */ 0.f, 0.f, 0.f,
         /* TR        */ 1.f, 0.f, 1.f, /* TL(0,0,1) */ 0.f, 0.f, 1.f},
    };

    constexpr static float FACE_UVS[12]{0.f, 0.f, 1.f, 0.f, 1.f, 1.f,
                                        0.f, 0.f, 1.f, 1.f, 0.f, 1.f};
}; // struct Block
} // namespace hi