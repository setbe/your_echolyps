#pragma once

#include "camera.hpp"
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

    Camera camera;
    Terrain terrain;

    inline explicit World() noexcept : terrain{}, camera{} {
        terrain.generate();
        terrain.upload();

        camera.position[0] = 45.f;
        camera.position[1] = 45.f;
        camera.position[2] = 114.f;
    }

    inline ~World() noexcept {}

    World(const World &) = delete;
    World &operator=(const World &) = delete;
    World(World &&) = delete;
    World &operator=(World &&) = delete;

    inline void camera_rotate(int xoffset, int yoffset) noexcept {
        camera.process_mouse_movement(xoffset, yoffset);
        camera.look_at(view);
    }
    inline unsigned draw() const noexcept {
        return terrain.draw(projection, view);
    }
};

} // namespace hi
