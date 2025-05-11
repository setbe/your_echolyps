#pragma once

#include "../engine/camera.hpp"
#include "../engine/opengl.hpp"

#include "../external/linmath.hpp"
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
        terrain.gen_data_all();
        terrain.gen_mesh_all();
        terrain.upload();

        camera.position[0] = Terrain::CHUNKS_X * Chunk::WIDTH / 2;
        camera.position[1] = 40.f;
        camera.position[2] = Terrain::CHUNKS_Z * Chunk::DEPTH / 2;
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
        camera.process_mouse_movement(static_cast<float>(xoffset),
                                      static_cast<float>(yoffset));
        camera.look_at(view);
    }

    inline void draw() const noexcept {
        terrain.draw(projection, view, camera.position);
    }
};

} // namespace hi
