#include "terrain.hpp"
#include "../resources/shaders.hpp"
#include "block_list.hpp"

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
    mesh_buffer =
        static_cast<Vertex *>(hi::alloc(TOTAL_VERT_CAP * sizeof(Vertex)));
    if (!mesh_buffer)
        panic(Result{Stage::Game, Error::ChunkMemoryAlloc});

    vbo.bind(GL_ARRAY_BUFFER);
    vbo.buffer_data(GL_ARRAY_BUFFER, TOTAL_VERT_CAP * sizeof(Vertex), nullptr,
                    GL_STATIC_DRAW);

    vao.bind();
    vbo.bind(GL_ARRAY_BUFFER);
    bind_vertex_attributes();

    shader_program.use();
    projection_location =
        glGetUniformLocation(shader_program.get(), "projection");
    view_location = glGetUniformLocation(shader_program.get(), "view");
    atlas_location = glGetUniformLocation(shader_program.get(), "atlas");

    constexpr unsigned TEX_SIZE =
        TEXTUREPACK_ATLAS_WIDTH * TEXTUREPACK_ATLAS_HEIGHT * 4;
    unsigned char *atlas_pixels =
        static_cast<unsigned char *>(hi::alloc(TEX_SIZE));
    if (!atlas_pixels)
        panic(Result{Stage::Game, Error::TexturepackMemoryAlloc});
    decompress_atlas(atlas_pixels);

    atlas.bind(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTUREPACK_ATLAS_WIDTH,
                 TEXTUREPACK_ATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 atlas_pixels);
    hi::free(atlas_pixels, TEX_SIZE);

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
    hi::free(mesh_buffer, TOTAL_VERT_CAP * sizeof(Vertex));
}

void Terrain::bind_vertex_attributes() const noexcept {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, position_block));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, uv));
}

void Terrain::generate_mesh_for(const Key &key, const Block *blocks,
                                std::vector<Vertex> &out) const noexcept {
    const unsigned W = Chunk::WIDTH, H = Chunk::HEIGHT, D = Chunk::DEPTH;

    std::unordered_map<Key, std::unique_ptr<Block[]>, Key::Hash> temp_neighbors;

    auto get_block = [&](int x, int y, int z) -> const Block * {
        return get_block_at_extended(key, blocks, x, y, z, temp_neighbors);
    };

    for (unsigned z = 0; z < D; ++z)
        for (unsigned y = 0; y < H; ++y)
            for (unsigned x = 0; x < W; ++x) {
                const Block &blk = blocks[z * W * H + y * W + x];
                if (blk.block_id() == BlockList::Air.block_id())
                    continue;

                int gx = x + key.x * W, gy = y + key.y * H, gz = z + key.z * D;

                for (int face = 0; face < 6; ++face) {
                    int dx = (face == 2) ? -1 : (face == 3) ? 1 : 0;
                    int dy = (face == 5) ? -1 : (face == 4) ? 1 : 0;
                    int dz = (face == 1) ? -1 : (face == 0) ? 1 : 0;
                    const Block *neighbour = get_block(x + dx, y + dy, z + dz);

                    const uint16_t neighbour_id = neighbour->block_id();
                    constexpr uint16_t air_id = BlockList::Air.block_id();
                    constexpr uint16_t water_id = BlockList::Water.block_id();
                    if (neighbour && (neighbour_id != air_id))
                        continue;

                    push_face(out, blk, gx, gy, gz, face);
                }
            }
}

const Block *Terrain::get_block_at_extended(
    const Key &center, const Block *blocks, int x, int y, int z,
    std::unordered_map<Key, std::unique_ptr<Block[]>, Key::Hash>
        &temp_neighbors) const noexcept {
    const int W = Chunk::WIDTH, H = Chunk::HEIGHT, D = Chunk::DEPTH;

    if (x >= 0 && x < W && y >= 0 && y < H && z >= 0 && z < D)
        return &blocks[z * W * H + y * W + x];

    int nx = x, ny = y, nz = z;
    Key nk = center;

    if (nx < 0) {
        nk.x--;
        nx += W;
    } else if (nx >= W) {
        nk.x++;
        nx -= W;
    }
    if (ny < 0) {
        nk.y--;
        ny += H;
    } else if (ny >= H) {
        nk.y++;
        ny -= H;
    }
    if (nz < 0) {
        nk.z--;
        nz += D;
    } else if (nz >= D) {
        nk.z++;
        nz -= D;
    }

    auto it = block_map.find(nk);
    const Block *neighbor = nullptr;

    if (it != block_map.end()) {
        neighbor = it->second.get();
    } else {
        auto temp_it = temp_neighbors.find(nk);
        if (temp_it == temp_neighbors.end()) {
            auto tmp = std::make_unique<Block[]>(Chunk::BLOCKS_PER_CHUNK);
            Chunk::generate_chunk(nk.x, nk.y, nk.z, tmp.get(), noise);
            neighbor = tmp.get();
            temp_neighbors.emplace(nk, std::move(tmp));
        } else {
            neighbor = temp_it->second.get();
        }
    }

    return &neighbor[nz * W * H + ny * W + nx];
}

void Terrain::push_face(std::vector<Vertex> &out, const Block &blk, int gx,
                        int gy, int gz, int face) const noexcept {
    using Tex = Block::TextureProtocol;

    constexpr int TPR = TEXTUREPACK_ATLAS_WIDTH / Tex::RESOLUTION;

    uint32_t bid = blk.block_id() - 1;
    uint32_t off = Tex::resolve_offset(blk.texture_protocol(), face);

    uint32_t tex = bid + off;
    uint32_t tx = tex % TPR, ty = tex / TPR;

    for (int i = 0; i < 6; ++i) {
        Vertex v;
        const float *POS = Block::CUBE_POS[face];
        v.position_block[0] = gx + POS[i * 3 + 0];
        v.position_block[1] = gy + POS[i * 3 + 1];
        v.position_block[2] = gz + POS[i * 3 + 2];
        v.position_block[3] = blk.to_float();
        v.uv[0] = (tx + Block::FACE_UVS[i * 2 + 0]) / float(TPR);
        v.uv[1] = (ty + Block::FACE_UVS[i * 2 + 1]) / float(TPR);
        out.push_back(v);
    }
}

void Terrain::request_chunk(const Key &key, int center_x, int center_y,
                            int center_z) {
    std::lock_guard lk(mutex_pending);
    if (!block_map.contains(key) && !pending_set.contains(key)) {
        int dist = std::abs(center_x - key.x) + std::abs(center_y - key.y) +
                   std::abs(center_z - key.z);
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

        memcpy(mesh_buffer + offset, verts.data(),
               verts.size() * sizeof(Vertex));

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

void Terrain::draw(const math::mat4x4 projection,
                   const math::mat4x4 view) const noexcept {
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

    // render chunks
    for (const auto &[key, mesh] : mesh_map) {
        if (!Chunk::is_chunk_visible(mesh, frustum_planes) ||
            mesh.vertex_count == 0)
            continue;
        glDrawArrays(GL_TRIANGLES, mesh.vertex_offset, mesh.vertex_count);
    }
}

} // namespace hi
