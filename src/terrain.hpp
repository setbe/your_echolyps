#pragma once

#include "chunk.hpp"
#include "external/linmath.hpp"
#include "opengl.hpp"
#include <assert.h>
#include <stdint.h>
#include <string.h>

namespace hi {

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct ChunkMesh {
    uint32_t vertex_offset;
    uint32_t vertex_count;
};

struct Terrain {
  private:
    Block *data;
    Vertex *mesh_buffer;

    constexpr static unsigned CHUNKS_PER_X = 10;
    constexpr static unsigned CHUNKS_PER_Y = 1;
    constexpr static unsigned CHUNKS_PER_Z = 10;
    constexpr static unsigned CHUNKS_COUNT =
        CHUNKS_PER_X * CHUNKS_PER_Y * CHUNKS_PER_Z;
    constexpr static unsigned BLOCKS_TOTAL =
        Chunk::BLOCKS_PER_CHUNK * CHUNKS_COUNT;
    constexpr static unsigned MAX_VERTICES = 10'000'000;

    Opengl::VBO vbo;
    Opengl::VAO vao;
    Opengl::ShaderProgram shader_program;
    unsigned projection_location;
    unsigned view_location;

    ChunkMesh chunk_meshes[CHUNKS_COUNT];
    uint32_t used_vertices = 0;

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

    inline Block get_block_at(int gx, int gy, int gz) const noexcept {
        if (gx < 0 || gy < 0 || gz < 0 || gx >= CHUNKS_PER_X * Chunk::WIDTH ||
            gy >= CHUNKS_PER_Y * Chunk::HEIGHT ||
            gz >= CHUNKS_PER_Z * Chunk::DEPTH)
            return {0, 0, 0};

        unsigned cx = gx / Chunk::WIDTH;
        unsigned cy = gy / Chunk::HEIGHT;
        unsigned cz = gz / Chunk::DEPTH;
        unsigned chunk_index =
            cx + cy * CHUNKS_PER_X + cz * CHUNKS_PER_X * CHUNKS_PER_Y;

        unsigned lx = gx % Chunk::WIDTH;
        unsigned ly = gy % Chunk::HEIGHT;
        unsigned lz = gz % Chunk::DEPTH;

        return data[chunk_index * Chunk::BLOCKS_PER_CHUNK +
                    Chunk::get_block_index(lx, ly, lz)];
    }

    inline void emit_face(unsigned &idx, float x, float y, float z,
                          int face_id) {
        const float *face = cube_faces[face_id];
        const float *n = normals[face_id];
        for (int i = 0; i < 6; ++i) {
            Vertex &v = mesh_buffer[idx++];
            v.x = x + face[i * 3 + 0];
            v.y = y + face[i * 3 + 1];
            v.z = z + face[i * 3 + 2];
            v.nx = n[0];
            v.ny = n[1];
            v.nz = n[2];
            v.u = (i == 0 || i == 3 || i == 5) ? 0.f : 1.f;
            v.v = (i == 0 || i == 1 || i == 4) ? 0.f : 1.f;
        }
    }

  public:
    consteval static unsigned get_size() noexcept {
        return CHUNKS_COUNT * Chunk::BLOCKS_PER_CHUNK * sizeof(Block);
    }

    inline explicit Terrain() noexcept
        : shader_program{terrain_vert, terrain_frag} {
        data = static_cast<Block *>(hi::alloc(get_size()));
        mesh_buffer =
            static_cast<Vertex *>(hi::alloc(MAX_VERTICES * sizeof(Vertex)));
        if (!data || !mesh_buffer)
            panic(Result{Stage::Game, Error::ChunkMemoryAlloc});

        vao.bind();
        vbo.bind(GL_ARRAY_BUFFER);
        vbo.buffer_data(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr,
                        GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(6 * sizeof(float)));

        shader_program.use();
        projection_location =
            glGetUniformLocation(shader_program.get(), "projection");
        view_location = glGetUniformLocation(shader_program.get(), "view");
    }

    inline ~Terrain() noexcept {
        hi::free(data, get_size());
        hi::free(mesh_buffer, MAX_VERTICES * sizeof(Vertex));
    }

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;
    Terrain(Terrain &&) = delete;
    Terrain &operator=(Terrain &&) = delete;

    inline void insert(unsigned index, Block block) noexcept {
        assert(index < BLOCKS_TOTAL);
        data[index] = block;
    }

    inline void upload() noexcept {
        vbo.bind(GL_ARRAY_BUFFER);
        vbo.sub_data(GL_ARRAY_BUFFER, 0, used_vertices * sizeof(Vertex),
                     mesh_buffer);
    }

    inline void draw(const math::mat4x4 projection,
                     const math::mat4x4 view) const noexcept {
        shader_program.use();
        vao.bind();
        glUniformMatrix4fv(projection_location, 1, GL_FALSE,
                           (const GLfloat *)projection);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, (const GLfloat *)view);
        for (unsigned i = 0; i < CHUNKS_COUNT; ++i) {
            const auto &mesh = chunk_meshes[i];
            glDrawArrays(GL_TRIANGLES, mesh.vertex_offset, mesh.vertex_count);
        }
    }

    inline void generate_chunk(unsigned chunk_index) noexcept {
        assert(chunk_index < CHUNKS_COUNT);
        Chunk::generate_chunk(chunk_index, data);

        unsigned base_vertex = used_vertices;
        unsigned count = 0;

        unsigned chunk_base = chunk_index * Chunk::BLOCKS_PER_CHUNK;
        unsigned cx = chunk_index % CHUNKS_PER_X;
        unsigned cy = (chunk_index / CHUNKS_PER_X) % CHUNKS_PER_Y;
        unsigned cz = chunk_index / (CHUNKS_PER_X * CHUNKS_PER_Y);

        for (unsigned z = 0; z < Chunk::DEPTH; ++z) {
            for (unsigned y = 0; y < Chunk::HEIGHT; ++y) {
                for (unsigned x = 0; x < Chunk::WIDTH; ++x) {
                    unsigned local_index = Chunk::get_block_index(x, y, z);
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
                            emit_face(used_vertices, gx, gy, gz, face);
                            count += 6;
                        }
                    }
                }
            }
        }
        chunk_meshes[chunk_index] = {base_vertex, count};
    }

    inline void generate() noexcept {
        for (unsigned z = 0; z < CHUNKS_PER_Z; ++z) {
            for (unsigned y = 0; y < CHUNKS_PER_Y; ++y) {
                for (unsigned x = 0; x < CHUNKS_PER_X; ++x) {
                    generate_chunk(x + y * CHUNKS_PER_X +
                                   z * CHUNKS_PER_X * CHUNKS_PER_Y);
                }
            }
        }
    }

    inline const Block *get_data() const noexcept { return data; }
};

} // namespace hi
