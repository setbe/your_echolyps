#include "terrain.hpp"

namespace hi {

Terrain::Terrain() noexcept : shader_program{terrain_vert, terrain_frag} {
    data = static_cast<Block *>(hi::alloc(get_size()));
    mesh_buffer = static_cast<Vertex *>(hi::alloc(current_buffer_capacity));
    index_buffer = static_cast<GLuint *>(
        hi::alloc(current_index_capacity * sizeof(GLuint)));

    if (!data || !mesh_buffer || !index_buffer)
        panic(Result{Stage::Game, Error::ChunkMemoryAlloc});

    vbo.bind(GL_ARRAY_BUFFER);
    vbo.buffer_data(GL_ARRAY_BUFFER, current_buffer_capacity, nullptr,
                    GL_DYNAMIC_DRAW);

    ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
    ebo.buffer_data(GL_ELEMENT_ARRAY_BUFFER,
                    current_index_capacity * sizeof(GLuint), nullptr,
                    GL_DYNAMIC_DRAW);

    vao.bind();
    vbo.bind(GL_ARRAY_BUFFER);
    bind_vertex_attributes();

    shader_program.use();
    projection_location =
        glGetUniformLocation(shader_program.get(), "projection");
    view_location = glGetUniformLocation(shader_program.get(), "view");
}

Terrain::~Terrain() noexcept {
    hi::free(data, get_size());
    hi::free(mesh_buffer, current_buffer_capacity);
    hi::free(index_buffer, current_index_capacity * sizeof(GLuint));
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
        Vertex &v = mesh_buffer[idx];
        v.position_block[0] = x + face[i * 3 + 0];
        v.position_block[1] = y + face[i * 3 + 1];
        v.position_block[2] = z + face[i * 3 + 2];
        v.position_block[3] = packed;
        index_buffer[used_indices++] = idx++;
    }
}

void Terrain::generate_block_data() noexcept {
    for (unsigned cz = 0; cz < CHUNKS_PER_Z; ++cz)
        for (unsigned cy = 0; cy < CHUNKS_PER_Y; ++cy)
            for (unsigned cx = 0; cx < CHUNKS_PER_X; ++cx) {
                unsigned i =
                    cx + cy * CHUNKS_PER_X + cz * CHUNKS_PER_X * CHUNKS_PER_Y;
                Block *dst = &data[i * Chunk::BLOCKS_PER_CHUNK];
                Chunk::generate_chunk(cx, cy, cz, dst, noise);
            }
}

void Terrain::generate_chunk_mesh(unsigned chunk_index) noexcept {
    unsigned base_vertex = used_vertices;
    unsigned count = 0;

    unsigned chunk_base = chunk_index * Chunk::BLOCKS_PER_CHUNK;
    unsigned cx = chunk_index % CHUNKS_PER_X;
    unsigned cy = (chunk_index / CHUNKS_PER_X) % CHUNKS_PER_Y;
    unsigned cz = chunk_index / (CHUNKS_PER_X * CHUNKS_PER_Y);

    for (unsigned z = 0; z < Chunk::DEPTH; ++z) {
        for (unsigned y = 0; y < Chunk::HEIGHT; ++y) {
            for (unsigned x = 0; x < Chunk::WIDTH; ++x) {
                unsigned local_index = Chunk::calculate_block_index(x, y, z);
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
                    Block neighbor = get_block_at(gx + dx, gy + dy, gz + dz);
                    if (neighbor.id == 0) {
                        emit_face(used_vertices, gx, gy, gz, face, blk);
                        count += 6;
                    }
                }
            }
        }
    }

    chunk_meshes[chunk_index] = {base_vertex, count, float(cx * Chunk::WIDTH),
                                 float(cy * Chunk::HEIGHT),
                                 float(cz * Chunk::DEPTH)};
}

void Terrain::generate() noexcept {
    used_vertices = 0;
    used_indices = 0;
    for (unsigned i = 0; i < CHUNKS_COUNT; ++i)
        generate_chunk_mesh(i);
}

void Terrain::upload() noexcept {
    vbo.bind(GL_ARRAY_BUFFER);
    vbo.sub_data(GL_ARRAY_BUFFER, 0, used_vertices * sizeof(Vertex),
                 mesh_buffer);
    ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
    ebo.buffer_data(GL_ELEMENT_ARRAY_BUFFER, used_indices * sizeof(GLuint),
                    index_buffer, GL_DYNAMIC_DRAW);
}

void Terrain::upload_block_data_to_ssbo() noexcept {
    ssbo.bind(GL_SHADER_STORAGE_BUFFER);
    ssbo.buffer_data(GL_SHADER_STORAGE_BUFFER, used_vertices * sizeof(Vertex),
                     mesh_buffer, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo.get());
}

void Terrain::draw(const math::mat4x4 projection, const math::mat4x4 view,
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

    ebo.bind(GL_ELEMENT_ARRAY_BUFFER);

    for (unsigned i = 0; i < CHUNKS_COUNT; ++i) {
        const auto &mesh = chunk_meshes[i];
        if (!Chunk::is_chunk_visible(mesh, planes))
            continue;
        glDrawElementsBaseVertex(
            GL_TRIANGLES, mesh.vertex_count, GL_UNSIGNED_INT,
            (void *)(mesh.vertex_offset * sizeof(GLuint)), 0);
    }
}

} // namespace hi
