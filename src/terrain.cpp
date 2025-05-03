#include "terrain.hpp"

namespace hi {
Terrain::Terrain() noexcept : shader_program{terrain_vert, terrain_frag} {
    data = static_cast<Block *>(hi::alloc(get_size()));
    mesh_buffer = static_cast<Vertex *>(hi::alloc(LOD0_SIZE));
    mesh_buffer_lod2 = static_cast<Vertex *>(hi::alloc(LOD2_SIZE));
    mesh_buffer_lod4 = static_cast<Vertex *>(hi::alloc(LOD4_SIZE));
    mesh_buffer_lod8 = static_cast<Vertex *>(hi::alloc(LOD8_SIZE));
    if (!data || !mesh_buffer || !mesh_buffer_lod2 || !mesh_buffer_lod4 ||
        !mesh_buffer_lod8)
        panic(Result{Stage::Game, Error::ChunkMemoryAlloc});

    vao.bind();
    vbo.bind(GL_ARRAY_BUFFER);
    vbo.buffer_data(GL_ARRAY_BUFFER, LOD0_SIZE, nullptr, GL_DYNAMIC_DRAW);

    vbo_lod2.bind(GL_ARRAY_BUFFER);
    vbo_lod2.buffer_data(GL_ARRAY_BUFFER, LOD2_SIZE, nullptr, GL_DYNAMIC_DRAW);
    vbo_lod4.bind(GL_ARRAY_BUFFER);
    vbo_lod4.buffer_data(GL_ARRAY_BUFFER, LOD4_SIZE, nullptr, GL_DYNAMIC_DRAW);
    vbo_lod8.bind(GL_ARRAY_BUFFER);
    vbo_lod8.buffer_data(GL_ARRAY_BUFFER, LOD8_SIZE, nullptr, GL_DYNAMIC_DRAW);
    bind_vertex_attributes();

    shader_program.use();
    projection_location =
        glGetUniformLocation(shader_program.get(), "projection");
    view_location = glGetUniformLocation(shader_program.get(), "view");
}

Terrain::~Terrain() noexcept {
    hi::free(data, get_size());
    hi::free(mesh_buffer, LOD0_SIZE);
    hi::free(mesh_buffer_lod2, LOD2_SIZE);
    hi::free(mesh_buffer_lod4, LOD4_SIZE);
    hi::free(mesh_buffer_lod8, LOD8_SIZE);
}

Block Terrain::get_block_at(int gx, int gy, int gz) const noexcept {
    if (gx < 0 || gy < 0 || gz < 0 || gx >= CHUNKS_PER_X * Chunk::WIDTH ||
        gy >= CHUNKS_PER_Y * Chunk::HEIGHT || gz >= CHUNKS_PER_Z * Chunk::DEPTH)
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
                Chunk::calculate_block_index(lx, ly, lz)];
}

void Terrain::emit_face(unsigned &idx, float x, float y, float z, int face_id,
                        Block block) noexcept {
    const float *face = cube_faces[face_id];
    float packed = block.to_float();

    for (int i = 0; i < 6; ++i) {
        Vertex &v = mesh_buffer[idx++];
        v.position_block[0] = x + face[i * 3 + 0];
        v.position_block[1] = y + face[i * 3 + 1];
        v.position_block[2] = z + face[i * 3 + 2];
        v.position_block[3] = packed;
    }
}

} // namespace hi