#pragma once

#include "chunk.hpp"
#include "external/PerlinNoise.hpp"
#include "external/linmath.hpp"
#include "opengl.hpp"
#include <assert.h>
#include <stdint.h>

namespace hi {

struct Vertex {
    math::vec4 position_block; // .xyz = pos, .w = packed block info
};

struct Terrain {
    siv::PerlinNoise noise{};

  private:
    static constexpr unsigned CHUNKS_PER_X = 64;
    static constexpr unsigned CHUNKS_PER_Y = 12;
    static constexpr unsigned CHUNKS_PER_Z = 64;
    static constexpr unsigned CHUNKS_COUNT =
        CHUNKS_PER_X * CHUNKS_PER_Y * CHUNKS_PER_Z;

    static constexpr size_t BLOCKS_TOTAL =
        Chunk::BLOCKS_PER_CHUNK * CHUNKS_COUNT;
    static constexpr size_t MAX_VERTEX_COUNT = BLOCKS_TOTAL * 6 / 2;

    Block *data;
    Vertex *mesh_buffer;
    GLuint *index_buffer;

    size_t current_buffer_capacity = MAX_VERTEX_COUNT * sizeof(Vertex);
    size_t current_index_capacity = MAX_VERTEX_COUNT;

    Opengl::VAO vao;
    Opengl::VBO vbo;
    Opengl::EBO ebo;
    Opengl::VBO ssbo;
    Opengl::ShaderProgram shader_program;
    unsigned projection_location;
    unsigned view_location;

    Chunk::Mesh chunk_meshes[CHUNKS_COUNT];
    uint32_t used_vertices = 0;
    uint32_t used_indices = 0;

    const float cube_faces[6][18] = {
        {0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1},
        {0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0},
        {1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1},
        {0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0},
        {0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1}};

    Block get_block_at(int gx, int gy, int gz) const noexcept;
    void emit_face(unsigned &idx, float x, float y, float z, int face_id,
                   Block block) noexcept;

  public:
    Terrain() noexcept;
    ~Terrain() noexcept;

    void generate_block_data() noexcept;
    void generate_chunk_mesh(unsigned chunk_index) noexcept;
    void generate() noexcept;
    void upload() noexcept;
    void draw(const math::mat4x4 projection, const math::mat4x4 view,
              const math::vec3 camera_pos) const noexcept;

    void upload_block_data_to_ssbo() noexcept;

    static constexpr size_t get_size() noexcept {
        return BLOCKS_TOTAL * sizeof(Block);
    }

    inline void bind_vertex_attributes() const noexcept {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)offsetof(Vertex, position_block));
    }
};

} // namespace hi
