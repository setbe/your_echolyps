#include "terrain.hpp"

#include "../resources/shaders.hpp"

#include "block_list.hpp"

#include <algorithm>

namespace hi {

bool Terrain::allocate_chunk_slot(GLuint count, GLuint &out_offset) {
    for (size_t i = 0; i < free_slots.size(); ++i) {
        if (free_slots[i].count >= count) {
            out_offset = free_slots[i].offset;
            if (free_slots[i].count == count) {
                free_slots.erase(free_slots.begin() + i);
            } else {
                free_slots[i].offset += count;
                free_slots[i].count -= count;
            }
            return true;
        }
    }

    if (used_vertices + count > TOTAL_VERT_CAP)
        return false;

    out_offset = used_vertices;
    used_vertices += count;
    return true;
}

void Terrain::free_chunk_slot(GLuint offset, GLuint count) {
    free_slots.push_back({offset, count});
}

Terrain::Terrain() noexcept : shader_program{terrain_vert, terrain_frag} {
    // vao, vbo
    vbo.bind(GL_ARRAY_BUFFER);
    vbo.buffer_data(GL_ARRAY_BUFFER, TOTAL_VERT_CAP * sizeof(Vertex), nullptr,
                    GL_STATIC_DRAW);

    vao.bind();
    vbo.bind(GL_ARRAY_BUFFER);
    bind_vertex_attributes();

    // shader program
    shader_program.use();
    projection_location =
        glGetUniformLocation(shader_program.get(), "projection");
    view_location = glGetUniformLocation(shader_program.get(), "view");
    atlas_location = glGetUniformLocation(shader_program.get(), "atlas");

    // prepare texture buffer
    constexpr unsigned TEX_SIZE =
        TEXTUREPACK_ATLAS_WIDTH * TEXTUREPACK_ATLAS_HEIGHT * 4;
    unsigned char *atlas_pixels =
        static_cast<unsigned char *>(hi::alloc(TEX_SIZE));
    if (!atlas_pixels)
        panic(Result{Stage::Game, Error::TexturepackMemoryAlloc});
    decompress_atlas(atlas_pixels);

    // create texture atlas
    atlas.bind(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTUREPACK_ATLAS_WIDTH,
                 TEXTUREPACK_ATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 atlas_pixels);
    hi::free(atlas_pixels, TEX_SIZE);

    pending_to_request.reserve(MAX_LOADED_CHUNKS);

    // give the job for the workers
    for (auto &worker : workers) {
        worker = std::thread([this] {
            while (running) {
                Key key;
                {
                    std::unique_lock lk(mutex_pending);
                    cv.wait(lk,
                            [&] { return !pending_queue.empty() || !running; });
                    if (!running)
                        break;
                    key = pending_queue.top().key;
                    pending_queue.pop();
                    pending_set.erase(key);
                }

                // Check old tasks
                Key center = center_chunk.load();
                int dist = std::abs(center.x - key.x) +
                           std::abs(center.y - key.y) +
                           std::abs(center.z - key.z);
                if (dist > STREAM_RADIUS)
                    continue;

                auto blocks =
                    std::make_unique<Block[]>(Chunk::BLOCKS_PER_CHUNK);
                Chunk::generate_chunk(key.x, key.y, key.z, blocks.get(), noise);
                std::vector<Vertex> mesh;
                mesh.reserve(2048);

                generate_mesh_for(key, blocks.get(), mesh);

                {
                    std::lock_guard lk_ready(mutex_ready);
                    ready.push({key, std::move(mesh)});
                }

                {
                    std::lock_guard lk_pending(mutex_pending);
                    block_map.emplace(key, std::move(blocks));
                }
            }
        });
    }
}

Terrain::~Terrain() noexcept {
    running = false;
    cv.notify_all();
    for (auto &worker : workers)
        if (worker.joinable())
            worker.join();
}

void Terrain::bind_vertex_attributes() const noexcept {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, position_block));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, uv));
}

void Terrain::request_chunk(const Key &key, int center_x, int center_y,
                            int center_z) {
    int dist = std::abs(center_x - key.x) + std::abs(center_y - key.y) +
               std::abs(center_z - key.z);
    if (dist > STREAM_RADIUS)
        return;
    std::lock_guard lk(mutex_pending);
    if (!block_map.contains(key) && !pending_set.contains(key)) {
        pending_queue.push(PrioritizedKey{dist, key});
        pending_set.insert(key);
        cv.notify_one();
    }
}

void Terrain::upload_ready_chunks() {
    std::lock_guard lk(mutex_ready);
    while (!ready.empty()) {
        auto [key, verts] = std::move(ready.front());
        ready.pop();

        GLuint offset = 0;
        if (!allocate_chunk_slot(verts.size(), offset)) {
            fprintf(stderr, "[ERROR] No space for chunk at (%d,%d,%d)\n", key.x,
                    key.y, key.z);
            continue;
        }

        vbo.bind(GL_ARRAY_BUFFER);
        vbo.sub_data(/* target */ GL_ARRAY_BUFFER,
                     /* offset */ offset * sizeof(Vertex),
                     /* size   */ verts.size() * sizeof(Vertex),
                     /* data   */ verts.data());

        mesh_map[key] =
            Chunk::Mesh{/* vertex_offset */ offset,
                        /* vertex_count  */ static_cast<unsigned>(verts.size()),
                        /* world_x       */ float(int(key.x * Chunk::WIDTH)),
                        /* world_y       */ float(int(key.y * Chunk::HEIGHT)),
                        /* world_z       */ float(int(key.z * Chunk::DEPTH))};
        loaded_chunks.insert(key);
    }
}

void Terrain::unload_chunks_not_in(
    const std::unordered_set<Key, Key::Hash> &active) {
    for (auto it = loaded_chunks.begin(); it != loaded_chunks.end();) {
        if (!active.contains(*it)) {
            const auto &mesh = mesh_map[*it];
            free_chunk_slot(mesh.vertex_offset, mesh.vertex_count);
            mesh_map.erase(*it);
            block_map.erase(*it);
            it = loaded_chunks.erase(it);
        } else {
            ++it;
        }
    }
}

inline float distance_squared(const math::vec3 a, float x, float y,
                              float z) noexcept {
    float dx = a[0] - x;
    float dy = a[1] - y;
    float dz = a[2] - z;
    return dx * dx + dy * dy + dz * dz;
}

void Terrain::draw(const math::mat4x4 projection, const math::mat4x4 view,
                   const math::vec3 camera_pos) const noexcept {
    shader_program.use();
    vao.bind();

    // texture atlas
    glActiveTexture(GL_TEXTURE1);
    atlas.bind(GL_TEXTURE_2D);
    /* atlas */ glUniform1i(atlas_location, 1);

    /* projection */ glUniformMatrix4fv(
        /* location  */ projection_location,
        /* count     */ 1,
        /* transpose */ GL_FALSE,
        /* value     */ (const GLfloat *)projection);

    /* view */ glUniformMatrix4fv(/* location  */ view_location,
                                  /* count     */ 1,
                                  /* transpose */ GL_FALSE,
                                  /* value     */ (const GLfloat *)view);

    // frustum culling
    float frustum_planes[6][4];
    {
        math::mat4x4 frustum_view;
        math::mat4x4_identity(frustum_view);
        math::mat4x4_mul(frustum_view, projection, view);
        Chunk::extract_frustum_planes(frustum_planes, frustum_view);
    }

    // sort chunks
    std::vector<const Chunk::Mesh *> drawlist;
    drawlist.reserve(mesh_map.size());

    for (const auto &[key, mesh] : mesh_map) {
        if (mesh.vertex_count == 0)
            continue;
        if (!Chunk::is_chunk_visible(mesh, frustum_planes))
            continue;

        drawlist.push_back(&mesh);
    }

    std::sort(drawlist.begin(), drawlist.end(),
              [&](const auto *a, const auto *b) {
                  float da = distance_squared(camera_pos, a->world_x,
                                              a->world_y, a->world_z);
                  float db = distance_squared(camera_pos, b->world_x,
                                              b->world_y, b->world_z);
                  return da < db; // або > для back-to-front
              });

    // render chunks
    for (const auto *mesh : drawlist) {
        glDrawArrays(GL_TRIANGLES, mesh->vertex_offset, mesh->vertex_count);
    }
}

void Terrain::update(int center_cx, int center_cy, int center_cz) noexcept {
    // 1. Formation of chunks by waves
    if (filling_pending) {

        int cx = pending_center.x;
        int cy = pending_center.y;
        int cz = pending_center.z;

        for (int dz = -wave_radius; dz <= wave_radius; ++dz)
            for (int dy = -wave_radius; dy <= wave_radius; ++dy)
                for (int dx = -wave_radius; dx <= wave_radius; ++dx) {
                    if (std::max({std::abs(dx), std::abs(dy), std::abs(dz)}) !=
                        wave_radius)
                        continue;

                    Chunk::Key key{cx + dx, cy + dy, cz + dz};
                    pending_to_request.push_back(key);
                }

        ++wave_radius;

        if (wave_radius > STREAM_RADIUS) {
            filling_pending = false;

            std::unordered_set<Chunk::Key, Chunk::Key::Hash> needed(
                pending_to_request.begin(), pending_to_request.end());
            if (loaded_chunks.size() > MAX_LOADED_CHUNKS)
                unload_chunks_not_in(needed);

            pending_index = 0;
        }
    }

    // 2. Requesting chunks to the queue
    constexpr int chunks_per_frame = 64;
    for (int i = 0;
         i < chunks_per_frame && pending_index < pending_to_request.size();
         ++i, ++pending_index) {
        const auto &key = pending_to_request[pending_index];
        request_chunk(key, center_cx, center_cy, center_cz);
    }

    // 3. Upload ready meshes
    upload_ready_chunks();
}

} // namespace hi
