#pragma once

#include "../engine/camera.hpp"
#include "../engine/opengl.hpp"

#include "../external/linmath.hpp"
#include "terrain.hpp"

#include <cmath>
#include <unordered_set>

namespace hi {

struct World {
    static constexpr int STREAM_RADIUS = 2;

    math::mat4x4 projection;
    math::mat4x4 view;

    Camera camera;
    Terrain terrain;

    int center_cx = -9999, center_cy = -9999, center_cz = -9999;

    World() noexcept : camera{}, terrain{} {
        camera.position[0] = 0.f;
        camera.position[1] = 60.f;
        camera.position[2] = 0.f;

        camera.look_at(view);
        math::mat4x4_perspective(projection, math::radians(camera.fov),
                                 1920.f / 1080.f, 0.1f, 512.f);

        update_pos();
    }

    void update_projection(int width, int height) noexcept {
        math::mat4x4_perspective(projection, math::radians(camera.fov),
                                 float(width) / float(height), 0.1f, 512.f);
    }

    void camera_rotate(int xoffset, int yoffset) noexcept {
        camera.process_mouse_movement(static_cast<float>(xoffset),
                                      static_cast<float>(yoffset));
        camera.look_at(view);
    }

    void update() noexcept { terrain.upload_ready_chunks(); }

    void draw() const noexcept {
        terrain.draw(projection, view, camera.position);
    }

    inline int chunk_coord(float pos, int size) const {
        return static_cast<int>(std::floor(pos / float(size)));
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

        std::unordered_set<ChunkKey, ChunkKey::Hash> needed;

        for (int dz = -STREAM_RADIUS; dz <= STREAM_RADIUS; ++dz)
            for (int dy = -STREAM_RADIUS; dy <= STREAM_RADIUS; ++dy)
                for (int dx = -STREAM_RADIUS; dx <= STREAM_RADIUS; ++dx) {
                    ChunkKey key{cx + dx, cy + dy, cz + dz};
                    needed.insert(key);
                    terrain.request_chunk(key);
                }

        terrain.unload_chunks_not_in(needed);
    }
};

} // namespace hi
