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

var Grass_Plains = make(T::grass_plains_top, THREE_SIDES);
var Dirt_Plains = make(T::grass_plains_bottom, ONE);

var Grass_Forest = make(T::grass_forest_top, THREE_SIDES);
var Dirt_Forest = make(T::grass_forest_bottom, ONE);

var Grass_Snow = make(T::grass_snow_top, THREE_SIDES);
var Dirt_Snow = make(T::grass_snow_bottom, ONE);

var Grass_Savannah = make(T::grass_savannah_top, THREE_SIDES);
var Dirt_Savannah = make(T::grass_savannah_bottom, ONE);

// BLOCK LIST END
// clang-format on

#undef var
}; // namespace BlockList
} // namespace hi