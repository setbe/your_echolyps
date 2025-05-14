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

struct ChunkKey {
    int x, y, z;
    bool operator==(const ChunkKey &o) const noexcept {
        return x == o.x && y == o.y && z == o.z;
    }
    struct Hash {
        std::size_t operator()(const ChunkKey &k) const noexcept {
            std::size_t h1 = std::hash<int>{}(k.x);
            std::size_t h2 = std::hash<int>{}(k.y);
            std::size_t h3 = std::hash<int>{}(k.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
};

struct FreeSlot {
    GLuint offset;
    GLuint count;
};

struct Terrain {
    static constexpr size_t TOTAL_VERT_CAP = 16 * 1024 * 1024;
    static constexpr size_t TOTAL_IDX_CAP = TOTAL_VERT_CAP;

    siv::PerlinNoise noise;

    Vertex *mesh_buffer = nullptr;
    GLuint *index_buffer = nullptr;

    Opengl::VAO vao;
    Opengl::VBO vbo;
    Opengl::EBO ebo;
    hi::Opengl::Texture atlas;
    Opengl::ShaderProgram shader_program;
    unsigned projection_location = 0;
    unsigned view_location = 0;
    unsigned atlas_location = 0;

    std::unordered_map<ChunkKey, std::unique_ptr<Block[]>, ChunkKey::Hash>
        block_map;
    std::unordered_map<ChunkKey, Chunk::Mesh, ChunkKey::Hash> mesh_map;
    std::unordered_set<ChunkKey, ChunkKey::Hash> loaded_chunks;

    std::vector<FreeSlot> free_slots;
    GLuint used_vertices = 0;
    GLuint used_indices = 0;

    std::queue<ChunkKey> pending_queue;
    std::unordered_set<ChunkKey, ChunkKey::Hash> pending_set;
    std::queue<std::pair<ChunkKey, std::vector<Vertex>>> ready;
    std::mutex mutex_pending;
    std::mutex mutex_ready;
    std::condition_variable cv;
    std::atomic<bool> running = true;
    std::thread worker;

    Terrain() noexcept;
    ~Terrain() noexcept;

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;

    void request_chunk(const ChunkKey &key);
    void upload_ready_chunks();
    void unload_chunks_not_in(
        const std::unordered_set<ChunkKey, ChunkKey::Hash> &active);
    void draw(const math::mat4x4 projection, const math::mat4x4 view,
              const math::vec3 camera_pos) const noexcept;

  private:
    void bind_vertex_attributes() const noexcept;
    void generate_mesh_for(const ChunkKey &key, const Block *blocks,
                           std::vector<Vertex> &out) const noexcept;
    bool allocate_chunk_slot(GLuint count, GLuint &out_offset);
    void free_chunk_slot(GLuint offset, GLuint count);
};

} // namespace hi
