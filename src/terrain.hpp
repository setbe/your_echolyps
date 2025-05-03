#pragma once

#include "chunk.hpp"
#include "external/linmath.hpp"
#include "opengl.hpp"
#include <assert.h>
#include <string.h>

namespace hi {

struct Vertex {
    math::vec4 position_block; // .xyz = pos, .w = packed block info
};

struct Terrain {
  private:
    using BlockGetterFn = Block (*)(void *userdata, int gx, int gy, int gz);

    siv::PerlinNoise noise{123456u};
    Block *data;
    Vertex *mesh_buffer;
    Vertex *mesh_buffer_lod2;
    Vertex *mesh_buffer_lod4;
    Vertex *mesh_buffer_lod8;

    constexpr static unsigned CHUNKS_PER_X = 32;
    constexpr static unsigned CHUNKS_PER_Y = 2;
    constexpr static unsigned CHUNKS_PER_Z = 32;
    constexpr static unsigned CHUNKS_COUNT =
        CHUNKS_PER_X * CHUNKS_PER_Y * CHUNKS_PER_Z;
    constexpr static unsigned BLOCKS_TOTAL =
        Chunk::BLOCKS_PER_CHUNK * CHUNKS_COUNT;
    constexpr static unsigned MAX_VERTICES = 13'000'000;

    constexpr static unsigned LOD0_SIZE = MAX_VERTICES * sizeof(Vertex);
    constexpr static unsigned LOD2_SIZE = MAX_VERTICES * sizeof(Vertex) / 8;
    constexpr static unsigned LOD4_SIZE = MAX_VERTICES * sizeof(Vertex) / 32;
    constexpr static unsigned LOD8_SIZE = MAX_VERTICES * sizeof(Vertex) / 64;

    float render_distance = 2048.f;

    Opengl::VAO vao;
    Opengl::VBO vbo;
    Opengl::VBO vbo_lod2;
    Opengl::VBO vbo_lod4;
    Opengl::VBO vbo_lod8;
    Opengl::ShaderProgram shader_program;
    unsigned projection_location;
    unsigned view_location;

    Chunk::Mesh chunk_meshes[CHUNKS_COUNT];
    Chunk::Mesh chunk_meshes_lod2[CHUNKS_COUNT];
    Chunk::Mesh chunk_meshes_lod4[CHUNKS_COUNT];
    Chunk::Mesh chunk_meshes_lod8[CHUNKS_COUNT];
    uint32_t used_vertices = 0;
    uint32_t used_vertices_lod2 = 0;
    uint32_t used_vertices_lod4 = 0;
    uint32_t used_vertices_lod8 = 0;

    const float cube_faces[6][18] = {
        {0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1}, // front (z+)
        {0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0}, // back (z-)
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0}, // left (x-)
        {1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1}, // right (x+)
        {0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0}, // top (y+)
        {0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1}  // bottom (y-)
    };

    const float normals[6][3] = {{0, 0, 1}, {0, 0, -1}, {-1, 0, 0},
                                 {1, 0, 0}, {0, 1, 0},  {0, -1, 0}};

    static Block terrain_block_getter(void *userdata, int gx, int gy,
                                      int gz) noexcept {
        return static_cast<const Terrain *>(userdata)->get_block_at(gx, gy, gz);
    }
    Block get_block_at(int gx, int gy, int gz) const noexcept;
    void emit_face(unsigned &idx, float x, float y, float z, int face_id,
                   Block this_block) noexcept;

  public:
    consteval static unsigned get_size() noexcept {
        return CHUNKS_COUNT * Chunk::BLOCKS_PER_CHUNK * sizeof(Block);
    }

    explicit Terrain() noexcept;
    ~Terrain() noexcept;

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;
    Terrain(Terrain &&) = delete;
    Terrain &operator=(Terrain &&) = delete;

    inline void insert(unsigned index, Block block) noexcept {
        assert(index < BLOCKS_TOTAL);
        data[index] = block;
    }

    inline void upload() const noexcept {
        assert(used_vertices * sizeof(Vertex) < LOD0_SIZE);
        assert(used_vertices_lod2 * sizeof(Vertex) < LOD2_SIZE);
        assert(used_vertices_lod4 * sizeof(Vertex) < LOD4_SIZE);
        assert(used_vertices_lod8 * sizeof(Vertex) < LOD8_SIZE);
        vbo.bind(GL_ARRAY_BUFFER);
        vbo.sub_data(GL_ARRAY_BUFFER, 0, used_vertices * sizeof(Vertex),
                     mesh_buffer);

        vbo_lod2.bind(GL_ARRAY_BUFFER);
        vbo_lod2.sub_data(GL_ARRAY_BUFFER, 0,
                          used_vertices_lod2 * sizeof(Vertex),
                          mesh_buffer_lod2);

        vbo_lod4.bind(GL_ARRAY_BUFFER);
        vbo_lod4.sub_data(GL_ARRAY_BUFFER, 0,
                          used_vertices_lod4 * sizeof(Vertex),
                          mesh_buffer_lod4);

        vbo_lod8.bind(GL_ARRAY_BUFFER);
        vbo_lod8.sub_data(GL_ARRAY_BUFFER, 0,
                          used_vertices_lod8 * sizeof(Vertex),
                          mesh_buffer_lod8);
    }

    inline unsigned draw(const math::mat4x4 projection, const math::mat4x4 view,
                         const math::vec3 camera_pos) const noexcept {
        shader_program.use();
        vao.bind();
        glUniformMatrix4fv(projection_location, 1, GL_FALSE,
                           (const GLfloat *)projection);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, (const GLfloat *)view);

        math::mat4x4 proj_view;
        math::mat4x4_mul(proj_view, projection, view);

        float planes[6][4];
        Chunk::extract_frustum_planes(planes, proj_view);

        unsigned skipped_chunks = 0;

        for (unsigned i = 0; i < CHUNKS_COUNT; ++i) {
            const auto &mesh0 = chunk_meshes[i];
            float cx = mesh0.world_x + Chunk::WIDTH * 0.5f;
            float cy = mesh0.world_y + Chunk::HEIGHT * 0.5f;
            float cz = mesh0.world_z + Chunk::DEPTH * 0.5f;

            float dx = cx - camera_pos[0];
            float dy = cy - camera_pos[1];
            float dz = cz - camera_pos[2];
            float dist_sq = dx * dx + dy * dy + dz * dz;

            const int lod = select_lod(dist_sq);

            const Chunk::Mesh *mesh = nullptr;
            const Opengl::VBO *vbo_ptr = nullptr;

            switch (lod) {
            case 0:
                mesh = &chunk_meshes[i];
                if (!Chunk::is_chunk_visible(*mesh, planes))
                    continue;
                vbo_ptr = &vbo;
                break;
            case 2:
                mesh = &chunk_meshes_lod2[i];
                if (!Chunk::is_chunk_visible(*mesh, planes))
                    continue;
                vbo_ptr = &vbo_lod2;
                break;
            case 4:
                mesh = &chunk_meshes_lod4[i];
                if (!Chunk::is_chunk_visible(*mesh, planes))
                    continue;
                vbo_ptr = &vbo_lod4;
                break;
            case 8:
            default:
                mesh = &chunk_meshes_lod8[i];
                if (!Chunk::is_chunk_visible(*mesh, planes))
                    continue;
                vbo_ptr = &vbo_lod8;
                break;
            }

            vbo_ptr->bind(GL_ARRAY_BUFFER);
            bind_vertex_attributes();
            glDrawArrays(GL_TRIANGLES, mesh->vertex_offset, mesh->vertex_count);
        }

        return skipped_chunks;
    }

    inline void generate_block_data() noexcept {
#ifndef NDEBUG
        double start_time = hi::time();
#endif // NDEBUG
        for (unsigned cz = 0; cz < CHUNKS_PER_Z; ++cz)
            for (unsigned cy = 0; cy < CHUNKS_PER_Y; ++cy)
                for (unsigned cx = 0; cx < CHUNKS_PER_X; ++cx) {
                    unsigned i = cx + cy * CHUNKS_PER_X +
                                 cz * CHUNKS_PER_X * CHUNKS_PER_Y;
                    Block *dst = &data[i * Chunk::BLOCKS_PER_CHUNK];
                    Chunk::generate_chunk(cx, cy, cz, dst, noise);
                }
#ifndef NDEBUG
        debug_print("block data time: %.2f\n", hi::time() - start_time);
#endif // NDEBUG
    }

    inline void generate_chunk_mesh(unsigned chunk_index) noexcept {
        assert(chunk_index < CHUNKS_COUNT);
        unsigned base_vertex = used_vertices;
        unsigned count = 0;

        unsigned chunk_base = chunk_index * Chunk::BLOCKS_PER_CHUNK;
        unsigned cx = chunk_index % CHUNKS_PER_X;
        unsigned cy = (chunk_index / CHUNKS_PER_X) % CHUNKS_PER_Y;
        unsigned cz = chunk_index / (CHUNKS_PER_X * CHUNKS_PER_Y);

        for (unsigned z = 0; z < Chunk::DEPTH; ++z) {
            for (unsigned y = 0; y < Chunk::HEIGHT; ++y) {
                for (unsigned x = 0; x < Chunk::WIDTH; ++x) {
                    unsigned local_index =
                        Chunk::calculate_block_index(x, y, z);
                    Block blk = data[chunk_base + local_index];
                    if (blk.id == 0)
                        continue;

                    int gx = x + cx * Chunk::WIDTH;
                    int gy = y + cy * Chunk::HEIGHT;
                    int gz = z + cz * Chunk::DEPTH;

                    for (int face = 0; face < 6; ++face) {
                        int dx = (face == 2) ? -1 : (face == 3) ? 1 : 0;
                        int dy = (face == 5) ? -1 : (face == 4) ? 1 : 0;
                        int dz = (face == 1) ? -1 : (face == 0) ? 1 : 0;
                        Block neighbor =
                            get_block_at(gx + dx, gy + dy, gz + dz);
                        if (neighbor.id == 0) {
                            emit_face(used_vertices, gx, gy, gz, face, blk);
                            count += 6;
                        }
                    }
                }
            }
        }

        chunk_meshes[chunk_index] = {
            base_vertex, count, float(cx * Chunk::WIDTH),
            float(cy * Chunk::HEIGHT), float(cz * Chunk::DEPTH)};
    }

    inline void generate_chunk_lod(Vertex *dst, uint32_t &used,
                                   unsigned world_offset_x,
                                   unsigned world_offset_y,
                                   unsigned world_offset_z, unsigned lod_size,
                                   Chunk::Mesh &mesh_out, void *userdata,
                                   BlockGetterFn get_block) noexcept {
        constexpr unsigned W = Chunk::WIDTH;
        constexpr unsigned H = Chunk::HEIGHT;
        constexpr unsigned D = Chunk::DEPTH;
        unsigned base_vertex = used;
        unsigned count = 0;

        Block blk{};

        for (unsigned z = 0; z < D; z += lod_size) {
            for (unsigned y = 0; y < H; y += lod_size) {
                for (unsigned x = 0; x < W; x += lod_size) {
                    bool solid = false;

                    for (unsigned dz = 0; dz < lod_size && z + dz < D; ++dz)
                        for (unsigned dy = 0; dy < lod_size && y + dy < H; ++dy)
                            for (unsigned dx = 0; dx < lod_size && x + dx < W;
                                 ++dx) {
                                int gx = int(world_offset_x + x + dx);
                                int gy = int(world_offset_y + y + dy);
                                int gz = int(world_offset_z + z + dz);
                                blk = get_block(userdata, gx, gy, gz);
                                if (blk.id != 0) {
                                    solid = true;
                                    goto emit;
                                }
                            }

                emit:
                    if (!solid)
                        continue;

                    float wx = float(world_offset_x + x);
                    float wy = float(world_offset_y + y);
                    float wz = float(world_offset_z + z);
                    float size = float(lod_size);

                    Block block = {1, 15, 0};
                    float packed = block.to_float();

                    for (int face = 0; face < 6; ++face) {
                        int dx = (face == 2)   ? -int(lod_size)
                                 : (face == 3) ? int(lod_size)
                                               : 0;
                        int dy = (face == 5)   ? -int(lod_size)
                                 : (face == 4) ? int(lod_size)
                                               : 0;
                        int dz = (face == 1)   ? -int(lod_size)
                                 : (face == 0) ? int(lod_size)
                                               : 0;

                        int gx = int(world_offset_x + x + dx);
                        int gy = int(world_offset_y + y + dy);
                        int gz = int(world_offset_z + z + dz);

                        bool neighbor_empty =
                            (get_block(userdata, gx, gy, gz).id == 0);
                        if (!neighbor_empty) {
                            bool is_edge = (x == 0 && dx < 0) ||
                                           (x + lod_size >= W && dx > 0) ||
                                           (y == 0 && dy < 0) ||
                                           (y + lod_size >= H && dy > 0) ||
                                           (z == 0 && dz < 0) ||
                                           (z + lod_size >= D && dz > 0);

                            if (!is_edge)
                                continue;

                            bool transparent_found = false;
                            for (unsigned dz_ = 0;
                                 dz_ < lod_size && !transparent_found; ++dz_)
                                for (unsigned dy_ = 0;
                                     dy_ < lod_size && !transparent_found;
                                     ++dy_)
                                    for (unsigned dx_ = 0;
                                         dx_ < lod_size && !transparent_found;
                                         ++dx_) {
                                        int ngx = gx + dx_;
                                        int ngy = gy + dy_;
                                        int ngz = gz + dz_;
                                        if (get_block(userdata, ngx, ngy, ngz)
                                                .id == 0) {
                                            transparent_found = true;
                                        }
                                    }

                            if (!transparent_found)
                                continue;
                        }

                        const float *face_data = cube_faces[face];
                        for (int i = 0; i < 6; ++i) {
                            Vertex &v = dst[used++];
                            const math::vec4 cooked_v{
                                wx + face_data[i * 3 + 0] * size,
                                wy + face_data[i * 3 + 1] * size,
                                wz + face_data[i * 3 + 2] * size, packed};
                            math::vec4_dup(v.position_block, cooked_v);
                        }
                        count += 6;
                    }
                }
            }
        }

        mesh_out = {base_vertex, count, float(world_offset_x),
                    float(world_offset_y), float(world_offset_z)};
    }

    inline void generate() noexcept {
        used_vertices = 0;
        used_vertices_lod2 = 0;
        used_vertices_lod4 = 0;
        used_vertices_lod8 = 0;

#ifndef NDEBUG
        double start_time = hi::time();
#endif // NDEBUG
        for (unsigned cz = 0; cz < CHUNKS_PER_Z; ++cz)
            for (unsigned cy = 0; cy < CHUNKS_PER_Y; ++cy)
                for (unsigned cx = 0; cx < CHUNKS_PER_X; ++cx) {
                    unsigned i = cx + cy * CHUNKS_PER_X +
                                 cz * CHUNKS_PER_X * CHUNKS_PER_Y;

                    const unsigned ox = cx * Chunk::WIDTH;
                    const unsigned oy = cy * Chunk::HEIGHT;
                    const unsigned oz = cz * Chunk::DEPTH;

                    Block *data_base = &data[i * Chunk::BLOCKS_PER_CHUNK];

                    // full detailization
                    Chunk::generate_chunk(cx, cy, cz, data_base, noise, 1);
                    generate_chunk_mesh(i);

                    // LOD2
                    Chunk::generate_chunk(cx, cy, cz, data_base, noise, 2);
                    generate_chunk_lod(mesh_buffer_lod2, used_vertices_lod2, ox,
                                       oy, oz, 2, chunk_meshes_lod2[i], this,
                                       terrain_block_getter);

                    // LOD4
                    Chunk::generate_chunk(cx, cy, cz, data_base, noise, 4);
                    generate_chunk_lod(mesh_buffer_lod4, used_vertices_lod4, ox,
                                       oy, oz, 4, chunk_meshes_lod4[i], this,
                                       terrain_block_getter);

                    // LOD8
                    Chunk::generate_chunk(cx, cy, cz, data_base, noise, 8);
                    generate_chunk_lod(mesh_buffer_lod8, used_vertices_lod8, ox,
                                       oy, oz, 8, chunk_meshes_lod8[i], this,
                                       terrain_block_getter);
                }

#ifndef NDEBUG
        printf("world gen time: %.2f\n", hi::time() - start_time);
#endif // NDEBUG

        debug_print("generated full vertices: %d\n"
                    "lod2: %d\n"
                    "lod4: %d\n"
                    "lod8: %d\n",
                    used_vertices, used_vertices_lod2, used_vertices_lod4,
                    used_vertices_lod8);
    }

    inline const Block *get_data() const noexcept { return data; }

    inline void bind_vertex_attributes() const noexcept {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)offsetof(Vertex, position_block));
    }

    inline int select_lod(float dist_sq) const noexcept {
        const float r0 = 64.f;
        const float r2 = 192.f;
        const float r4 = 230.f;
        const float r8 = render_distance;

        if (dist_sq < r0 * r0)
            return 0;
        if (dist_sq < r2 * r2)
            return 2;
        if (dist_sq < r4 * r4)
            return 4;
        return 8;
    }
};

} // namespace hi
