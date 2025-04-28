#pragma once

#include "higui/higui_platform.hpp"
#include "opengl.hpp"

struct World {

    inline explicit World() noexcept {}
    inline ~World() noexcept {}

    World(const World &) = delete;
    World &operator=(const World &) = delete;
    World(World &&) = delete;
    World &operator=(World &&) = delete;

    inline void draw() const noexcept {}
}; // struct World