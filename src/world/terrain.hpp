#pragma once

#include "../engine/opengl.hpp"
#include "chunk.hpp"

#include <array>
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
}; // struct Vertex

struct FreeSlot {
    GLuint offset;
    GLuint count;
}; // struct FreeSlot

struct PrioritizedKey {
    int priority; // less is better
    Chunk::Key key;

    bool operator<(const PrioritizedKey &rhs) const noexcept {
        // less priority handles earlier
        return priority > rhs.priority; // std::priority_queue — max heap
    }
}; // struct PrioritizedKey

struct Terrain {
    constexpr static unsigned char THREADS_NUM = 6;
    static constexpr int STREAM_RADIUS = 10;
    static constexpr size_t TOTAL_VERT_CAP = UINT32_MAX / sizeof(Vertex);
    // Chunk::BLOCKS_PER_CHUNK * (STREAM_RADIUS * 2) * (STREAM_RADIUS * 2) *
    // (STREAM_RADIUS * 2) * 6 /* Faces per cube */ / 4 /* Coefficient */;

    using Key = Chunk::Key;

    NoiseSystem noise;

    Vertex *mesh_buffer = nullptr;

    Opengl::VAO vao;
    Opengl::VBO vbo;
    hi::Opengl::Texture atlas;
    Opengl::ShaderProgram shader_program;
    unsigned projection_location = 0;
    unsigned view_location = 0;
    unsigned atlas_location = 0;

    std::unordered_map<Key, std::unique_ptr<Block[]>, Key::Hash> block_map;
    std::unordered_map<Key, Chunk::Mesh, Key::Hash> mesh_map;
    std::unordered_set<Key, Key::Hash> loaded_chunks;

    std::vector<FreeSlot> free_slots;
    GLuint used_vertices = 0;

    std::atomic<Chunk::Key> center_chunk;
    std::priority_queue<PrioritizedKey> pending_queue;
    std::unordered_set<Key, Key::Hash> pending_set;
    std::queue<std::pair<Key, std::vector<Vertex>>> ready;
    std::mutex mutex_pending;
    std::mutex mutex_ready;

    std::condition_variable cv;
    std::atomic<bool> running = true;
    std::array<std::thread, THREADS_NUM> workers;

    Terrain() noexcept;
    ~Terrain() noexcept;

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;

    void request_chunk(const Key &key, int center_x, int center_y,
                       int center_z);
    void upload_ready_chunks();
    void unload_chunks_not_in(const std::unordered_set<Key, Key::Hash> &active);
    void draw(const math::mat4x4 projection,
              const math::mat4x4 view) const noexcept;

  private:
    void bind_vertex_attributes() const noexcept;
    void generate_mesh_for(const Key &key, const Block *blocks,
                           std::vector<Vertex> &out) const noexcept;
    bool allocate_chunk_slot(GLuint count, GLuint &out_offset);
    void free_chunk_slot(GLuint offset, GLuint count);

    const Block *get_block_at_extended(
        const Key &center, const Block *blocks, int x, int y, int z,
        std::unordered_map<Key, std::unique_ptr<Block[]>, Key::Hash>
            &temp_neighbors) const noexcept;
    void push_face(std::vector<Vertex> &out, const Block &blk, int gx, int gy,
                   int gz, int face) const noexcept;
};

} // namespace hi
