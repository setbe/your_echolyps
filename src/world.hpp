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
        terrain.generate_block_data();
        terrain.generate();
        terrain.upload();

        camera.position[0] = 512.f;
        camera.position[1] = 50.f;
        camera.position[2] = 512.f;
    }

    inline ~World() noexcept {}

    World(const World &) = delete;
    World &operator=(const World &) = delete;
    World(World &&) = delete;
    World &operator=(World &&) = delete;

    inline void update_projection(int width, int height) noexcept {
        math::mat4x4_perspective(projection, math::radians(camera.fov),
                                 (float)width / (float)height, 0.1f, 512.f);
    }

    inline void camera_rotate(int xoffset, int yoffset) noexcept {
        camera.process_mouse_movement(xoffset, yoffset);
        camera.look_at(view);
    }
    inline void draw() const noexcept {
        terrain.draw(projection, view, camera.position);
    }
};

} // namespace hi
