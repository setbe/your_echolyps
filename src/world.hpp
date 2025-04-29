#pragma once

#include "external/linmath.hpp"
#include "higui/higui_platform.hpp"
#include "opengl.hpp"
#include "terrain.hpp"

namespace hi {

// ===== World: Operates with blocks and entities
// =====
struct World {
    math::mat4x4 projection;
    math::mat4x4 view;

    Terrain terrain;

    inline explicit World() noexcept : terrain{} {
        terrain.generate_chunk(terrain.get_chunk_index(0, 0, 0));
        terrain.upload();

        math::vec3 camera_pos = {-15.0f, 0.0f, -15.0f};
        math::vec3 target = {0.5f, 0.5f, 0.5f};
        math::vec3 up = {0.0f, 1.0f, 0.0f};

        math::mat4x4_identity(projection);
        math::mat4x4_perspective(projection, math::radians(70.f), 800.f / 600.f,
                                 0.1f, 100.f);

        math::mat4x4_look_at(view, camera_pos, target, up);
    }

    inline ~World() noexcept {}

    World(const World &) = delete;
    World &operator=(const World &) = delete;
    World(World &&) = delete;
    World &operator=(World &&) = delete;

    inline void draw() const noexcept { terrain.draw(projection, view); }
};

} // namespace hi
