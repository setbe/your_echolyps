#pragma once

#include "../engine/camera.hpp"
#include "../engine/opengl.hpp"

#include "../external/linmath.hpp"
#include "terrain.hpp"

#include <cmath>
#include <unordered_set>

namespace hi {

struct World {
    math::mat4x4 projection;
    math::mat4x4 view;

    Camera camera;
    Terrain terrain;

    int center_cx = -9999, center_cy = -9999, center_cz = -9999;

    int pos_update_cooldown = 0;
    constexpr static int POS_UPDATE_DELAY = 5;

    World() noexcept : camera{}, terrain{} {
        camera.position[0] = 0.f;
        camera.position[1] = 100.f;
        camera.position[2] = 0.f;
        update_pos();
    }

    void update_projection(int width, int height) noexcept {
        math::mat4x4_perspective(projection, math::radians(camera.fov),
                                 float(width) / float(height), 0.1f, 1024.f);
    }

    void camera_rotate(int xoffset, int yoffset) noexcept {
        camera.process_mouse_movement(static_cast<float>(xoffset),
                                      static_cast<float>(yoffset));
        camera.look_at(view);
    }

    void update() noexcept { terrain.update(center_cx, center_cy, center_cz); }

    void draw() const noexcept { terrain.draw(projection, view); }

    inline int chunk_coord(float pos, int size) const {
        return static_cast<int>(math::floorf(pos / float(size)));
    }

    void update_pos() {
        int cx = chunk_coord(camera.position[0], Chunk::WIDTH);
        int cy = chunk_coord(camera.position[1], Chunk::HEIGHT);
        int cz = chunk_coord(camera.position[2], Chunk::DEPTH);

        if (cx == center_cx && cy == center_cy && cz == center_cz)
            return;

        center_cx = cx;
        center_cy = cy;
        center_cz = cz;

        terrain.center_chunk.store(Chunk::Key{cx, cy, cz});

        terrain.pending_to_request.clear();
        terrain.pending_index = 0;
        terrain.wave_radius = 0;
        terrain.filling_pending = true;
        terrain.pending_center = Chunk::Key{cx, cy, cz};
    }
};

} // namespace hi
