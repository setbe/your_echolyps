#pragma once

#include "chunk.hpp"
#include "external/linmath.hpp"
#include "opengl.hpp"

#include <assert.h>
#include <stdint.h>
#include <string.h>

namespace hi {

// Will be placed in .rodata segment (flash/ROM)
static const float cube_vertices[] = {
    0, 0, 1, 1, 0, 1, 1, 1, 1, /**/ 0, 0, 1, 1, 1, 1, 0, 1, 1,  // front
    0, 0, 0, 0, 1, 0, 1, 1, 0, /**/ 0, 0, 0, 1, 1, 0, 1, 0, 0,  // back
    0, 0, 0, 0, 0, 1, 0, 1, 1, /**/ 0, 0, 0, 0, 1, 1, 0, 1, 0,  // left
    1, 0, 0, 1, 1, 0, 1, 1, 1, /**/ 1, 0, 0, 1, 1, 1, 1, 0, 1,  // right
    0, 1, 0, 0, 1, 1, 1, 1, 1, /**/ 0, 1, 0, 1, 1, 1, 1, 1, 0,  // top
    0, 0, 0, 1, 0, 0, 1, 0, 1, /**/ 0, 0, 0, 1, 0, 1, 0, 0, 1}; // bottom

// Chunk container
struct Terrain {
  private:
    Block *data;

    constexpr static unsigned CHUNKS_PER_X_DEFAULT = 256;
    constexpr static unsigned CHUNKS_PER_Y_DEFAULT = 16;
    constexpr static unsigned CHUNKS_PER_Z_DEFAULT = 256;

    constexpr static unsigned CHUNKS_PER_X = 1;
    constexpr static unsigned CHUNKS_PER_Y = 1;
    constexpr static unsigned CHUNKS_PER_Z = 1;
    constexpr static unsigned CHUNKS_COUNT =
        CHUNKS_PER_X * CHUNKS_PER_Y * CHUNKS_PER_Z;

    constexpr static unsigned BLOCKS_TOTAL =
        Chunk::BLOCKS_PER_CHUNK * CHUNKS_COUNT;

    Opengl::SSBO ssbo_vertices{};  // `cube_vertices` will be pushed here
    Opengl::SSBO ssbo_instances{}; // blocks data will be pushed here
    Opengl::VAO vao{};
    Opengl::ShaderProgram shader_program;
    unsigned projection_location;
    unsigned view_location;

  public:
    consteval static unsigned get_size() noexcept {
        return CHUNKS_COUNT * Chunk::BLOCKS_PER_CHUNK * sizeof(Block);
    }

    inline explicit Terrain() noexcept
        : shader_program{terrain_vert, terrain_frag} {
        data = static_cast<Block *>(hi::alloc(get_size()));
        if (!data)
            panic(Result{Stage::Game, Error::ChunkMemoryAlloc});

        vao.bind();
        ssbo_vertices.bind(GL_SHADER_STORAGE_BUFFER);
        ssbo_vertices.buffer_data(GL_SHADER_STORAGE_BUFFER,
                                  sizeof(cube_vertices), cube_vertices,
                                  GL_STATIC_DRAW);
        ssbo_vertices.buffer_base(GL_SHADER_STORAGE_BUFFER, 0);

        // glEnableVertexAttribArray(0);
        // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
        //                       (void *)0);

        shader_program.use();

        projection_location =
            glGetUniformLocation(shader_program.get(), "projection");
        view_location = glGetUniformLocation(shader_program.get(), "view");
    }
    inline ~Terrain() noexcept { hi::free((void *)get_data(), get_size()); }

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;
    Terrain(Terrain &&) = delete;
    Terrain &operator=(Terrain &&) = delete;

    inline const Block *get_data() const noexcept { return data; }

    inline unsigned get_chunk_index(unsigned x, unsigned y,
                                    unsigned z) const noexcept {
        assert(x < CHUNKS_PER_X && y < CHUNKS_PER_Y && z < CHUNKS_PER_Z);
        return x + y * CHUNKS_PER_X + z * CHUNKS_PER_X * CHUNKS_PER_Y;
    }

    inline void insert(unsigned index, Block block) noexcept {
        assert(index < BLOCKS_TOTAL);
        data[index] = block;
    }

    inline void draw(const math::mat4x4 projection,
                     const math::mat4x4 view) const noexcept {
        shader_program.use();
        vao.bind();

        glUniformMatrix4fv(projection_location, 1, GL_FALSE,
                           (const GLfloat *)projection);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, (const GLfloat *)view);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, BLOCKS_TOTAL);
    }

    inline void upload() const noexcept {
        vao.bind();
        ssbo_instances.bind(GL_SHADER_STORAGE_BUFFER);
        ssbo_instances.buffer_data(GL_SHADER_STORAGE_BUFFER, get_size(),
                                   (void *)get_data(), GL_STATIC_DRAW);
        ssbo_instances.buffer_base(GL_SHADER_STORAGE_BUFFER, 1);
    }

    inline void generate_chunk(unsigned chunk_index) noexcept {
        assert(chunk_index < CHUNKS_COUNT);
        Chunk::generate_chunk(chunk_index, data);
    }
};
} // namespace hi