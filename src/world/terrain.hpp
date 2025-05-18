#pragma once

#include "../engine/opengl.hpp"
#include "../external/PerlinNoise.hpp"
#include "../external/linmath.hpp"
#include "chunk.hpp"

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hi {

struct Vertex {
    math::vec4 position_block;
    math::vec2 uv;
};

struct FreeSlot {
    GLuint offset;
    GLuint count;
};

struct Terrain {
    static constexpr int STREAM_RADIUS = 6;
    static constexpr size_t TOTAL_VERT_CAP =
        Chunk::BLOCKS_PER_CHUNK * (STREAM_RADIUS * 2) * (STREAM_RADIUS * 2) *
        (STREAM_RADIUS * 2) * 6 /* Faces per cube */ / 4 /* Coefficient */;

    siv::PerlinNoise noise;

    Vertex *mesh_buffer = nullptr;

    Opengl::VAO vao;
    Opengl::VBO vbo;
    hi::Opengl::Texture atlas;
    Opengl::ShaderProgram shader_program;
    unsigned projection_location = 0;
    unsigned view_location = 0;
    unsigned atlas_location = 0;

    std::unordered_map<Chunk::Key, std::unique_ptr<Block[]>, Chunk::Key::Hash>
        block_map;
    std::unordered_map<Chunk::Key, Chunk::Mesh, Chunk::Key::Hash> mesh_map;
    std::unordered_set<Chunk::Key, Chunk::Key::Hash> loaded_chunks;

    std::vector<FreeSlot> free_slots;
    GLuint used_vertices = 0;

    std::queue<Chunk::Key> pending_queue;
    std::unordered_set<Chunk::Key, Chunk::Key::Hash> pending_set;
    std::queue<std::pair<Chunk::Key, std::vector<Vertex>>> ready;
    std::mutex mutex_pending;
    std::mutex mutex_ready;
    std::condition_variable cv;
    std::atomic<bool> running = true;
    std::thread worker;

    Terrain() noexcept;
    ~Terrain() noexcept;

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;

    void request_chunk(const Chunk::Key &key);
    void upload_ready_chunks();
    void unload_chunks_not_in(
        const std::unordered_set<Chunk::Key, Chunk::Key::Hash> &active);
    void draw(const math::mat4x4 projection,
              const math::mat4x4 view) const noexcept;

  private:
    void bind_vertex_attributes() const noexcept;
    void generate_mesh_for(const Chunk::Key &key, const Block *blocks,
                           std::vector<Vertex> &out) const noexcept;
    bool allocate_chunk_slot(GLuint count, GLuint &out_offset);
    void free_chunk_slot(GLuint offset, GLuint count);
};

} // namespace hi
