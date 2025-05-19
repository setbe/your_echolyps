#pragma once

#include "block.hpp"

#include "../resources/texturepack.hpp"

namespace hi {
namespace BlockList {

constexpr uint16_t ONE = Block::TextureProtocol::ONE;
constexpr uint16_t TWO = Block::TextureProtocol::TWO;
constexpr uint16_t THREE_FRONT = Block::TextureProtocol::THREE_FRONT;
constexpr uint16_t THREE_SIDES = Block::TextureProtocol::THREE_SIDES;
constexpr uint16_t SIX = Block::TextureProtocol::SIX;

constexpr Block make(Texturepack texture, uint16_t proto,
                     bool transparent = false) {
    return {Block::make_id(static_cast<uint16_t>(texture), proto), 8,
            0b1111'1100, transparent};
}

using T = Texturepack;
#define var constexpr Block

// clang-format off
// BLOCK LIST BEGIN

var Air =         {0, 8, 0b1111'1100, true};

var Cobblestone = make(T::cobblestone, ONE);
var Water = make(T::water, ONE, true);

var Grass = make(T::grass_top, THREE_SIDES);
var Dirt = make(T::grass_bottom, ONE);

// BLOCK LIST END
// clang-format on

#undef var
}; // namespace BlockList
} // namespace hi