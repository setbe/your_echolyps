#include "terrain.hpp"

#include "../resources/shaders.hpp" // for `terrain_vert`, `terrain_frag`
#include "../resources/texturepack.hpp"

namespace hi {

Terrain::Terrain() noexcept : shader_program{terrain_vert, terrain_frag} {
    data = static_cast<Block *>(hi::alloc(size()));
    mesh_buffer = static_cast<Vertex *>(hi::alloc(current_buffer_capacity));
    index_buffer = static_cast<GLuint *>(
        hi::alloc(current_index_capacity * sizeof(GLuint)));

    if (!data || !mesh_buffer || !index_buffer)
        panic(Result{Stage::Game, Error::ChunkMemoryAlloc});

    vbo.bind(GL_ARRAY_BUFFER);
    vbo.buffer_data(GL_ARRAY_BUFFER, current_buffer_capacity, nullptr,
                    GL_STATIC_DRAW);

    ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
    ebo.buffer_data(GL_ELEMENT_ARRAY_BUFFER,
                    current_index_capacity * sizeof(GLuint), nullptr,
                    GL_STATIC_DRAW);

    vao.bind();
    vbo.bind(GL_ARRAY_BUFFER);
    bind_vertex_attributes();

    shader_program.use();
    projection_location =
        glGetUniformLocation(shader_program.get(), "projection");
    view_location = glGetUniformLocation(shader_program.get(), "view");
    atlas_location = glGetUniformLocation(shader_program.get(), "atlas");

    constexpr unsigned TEXTUREPACK_SIZE =
        TEXTUREPACK_ATLAS_WIDTH * TEXTUREPACK_ATLAS_HEIGHT * 4;

    unsigned char *atlas_pixels =
        static_cast<unsigned char *>(hi::alloc(TEXTUREPACK_SIZE));
    if (!atlas_pixels)
        hi::panic(Result{Stage::Game, Error::TexturepackMemoryAlloc});

    decompress_atlas(atlas_pixels);

    atlas.bind(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTUREPACK_ATLAS_WIDTH,
                 TEXTUREPACK_ATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 atlas_pixels);

    hi::free(atlas_pixels, TEXTUREPACK_SIZE);
}

Terrain::~Terrain() noexcept {
    hi::free(data, size());
    hi::free(mesh_buffer, current_buffer_capacity);
    hi::free(index_buffer, current_index_capacity * sizeof(GLuint));
}

Block Terrain::get_block_at(int gx, int gy, int gz) const noexcept {
    // g - Global
    // c - Chunk
    // l - Local

    if (gx < 0 || gy < 0 || gz < 0 || gx >= CHUNKS_X * Chunk::WIDTH ||
        gy >= CHUNKS_Y * Chunk::HEIGHT || gz >= CHUNKS_Z * Chunk::DEPTH)
        return {0, 0, 0, 0};

    unsigned cx = gx / Chunk::WIDTH;
    unsigned cy = gy / Chunk::HEIGHT;
    unsigned cz = gz / Chunk::DEPTH;
    unsigned chunk_index = cx + cy * CHUNKS_X + cz * CHUNKS_X * CHUNKS_Y;

    unsigned lx = gx % Chunk::WIDTH;
    unsigned ly = gy % Chunk::HEIGHT;
    unsigned lz = gz % Chunk::DEPTH;

    return data[chunk_index * Chunk::BLOCKS_PER_CHUNK +
                Chunk::calculate_block_index(lx, ly, lz)];
}

void Terrain::emit_face(unsigned &idx, float x, float y, float z, int face,
                        Block block) noexcept {
    const float *POS = Block::CUBE_POS[face];

    uint32_t mask = block.texture_protocol();
    uint32_t bid = block.block_id() - 1;
    uint32_t off = 0;
    using Texture = BlockList::Texture;

    if (mask == Texture::ONE)
        off = 0;
    else if (mask == Texture::TWO)
        off = (face == 4 || face == 5) ? 0 : 1;
    else if (mask == Texture::THREE_FRONT)
        off = (face == 4 || face == 5) ? 0 : face == 0 ? 1 : 2;
    else if (mask == Texture::THREE_SIDES)
        off = face == 4 ? 0 : face == 5 ? 2 : 1;
    else
        off = face; // SIX

    constexpr int TILES_PER_ROW =
        TEXTUREPACK_ATLAS_WIDTH / BlockList::Texture::RESOLUTION;

    uint32_t tex = bid + off;
    uint32_t tx = tex % TILES_PER_ROW, ty = tex / TILES_PER_ROW;

    for (int i = 0; i < 6; ++i) {
        Vertex &vertex = mesh_buffer[idx];
        vertex.position_block[0] = x + POS[i * 3 + 0];
        vertex.position_block[1] = y + POS[i * 3 + 1];
        vertex.position_block[2] = z + POS[i * 3 + 2];
        vertex.position_block[3] = block.to_float();

        float u = static_cast<float>(tx) + Block::FACE_UVS[i * 2 + 0];
        float v = static_cast<float>(ty) + Block::FACE_UVS[i * 2 + 1];
        vertex.uv[0] = u / static_cast<float>(TILES_PER_ROW);
        vertex.uv[1] = v / static_cast<float>(TILES_PER_ROW);

        index_buffer[used_indices++] = idx++;
    }
}

void Terrain::bind_vertex_attributes() const noexcept {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, position_block));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, uv));
}

void Terrain::gen_data(unsigned cx, unsigned cy, unsigned cz) noexcept {
    unsigned chunk_index = cx + cy * CHUNKS_X + cz * CHUNKS_X * CHUNKS_Y;
    Block *dst = &data[chunk_index * Chunk::BLOCKS_PER_CHUNK];
    Chunk::generate_chunk(cx, cy, cz, dst, noise);
}
void Terrain::gen_data_all() noexcept {
    for (unsigned cz = 0; cz < CHUNKS_Z; ++cz)
        for (unsigned cy = 0; cy < CHUNKS_Y; ++cy)
            for (unsigned cx = 0; cx < CHUNKS_X; ++cx)
                gen_data(cx, cy, cz);
}

void Terrain::gen_mesh(unsigned chunk_index) noexcept {
    unsigned base_vertex = used_vertices;
    unsigned count = 0;

    unsigned c_base = chunk_index * Chunk::BLOCKS_PER_CHUNK;
    unsigned cx = chunk_index % CHUNKS_X;
    unsigned cy = (chunk_index / CHUNKS_X) % CHUNKS_Y;
    unsigned cz = chunk_index / (CHUNKS_X * CHUNKS_Y);

    for (unsigned z = 0; z < Chunk::DEPTH; ++z)
        for (unsigned y = 0; y < Chunk::HEIGHT; ++y)
            for (unsigned x = 0; x < Chunk::WIDTH; ++x) {
                unsigned local_index = Chunk::calculate_block_index(x, y, z);
                Block blk = data[c_base + local_index];
                if (blk.block_id() == BlockList::Air.block_id())
                    continue;

                int gx = x + cx * Chunk::WIDTH;
                int gy = y + cy * Chunk::HEIGHT;
                int gz = z + cz * Chunk::DEPTH;

                for (int face = 0; face < 6; ++face) {
                    int dx = (face == 2) ? -1 : (face == 3) ? 1 : 0;
                    int dy = (face == 5) ? -1 : (face == 4) ? 1 : 0;
                    int dz = (face == 1) ? -1 : (face == 0) ? 1 : 0;
                    Block neighbor = get_block_at(gx + dx, gy + dy, gz + dz);
                    if (neighbor.block_id() == 0) {
                        emit_face(used_vertices, gx, gy, gz, face, blk);
                        count += 6;
                    }
                }
            }

    chunk_meshes[chunk_index] = {base_vertex, count, float(cx * Chunk::WIDTH),
                                 float(cy * Chunk::HEIGHT),
                                 float(cz * Chunk::DEPTH)};
}

void Terrain::gen_mesh_all() noexcept {
    used_vertices = 0;
    used_indices = 0;
    for (unsigned i = 0; i < CHUNKS_COUNT; ++i)
        gen_mesh(i);
}

void Terrain::upload() noexcept {
    vbo.bind(GL_ARRAY_BUFFER);
    vbo.sub_data(GL_ARRAY_BUFFER, 0, used_vertices * sizeof(Vertex),
                 mesh_buffer);
    ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
    ebo.buffer_data(GL_ELEMENT_ARRAY_BUFFER, used_indices * sizeof(GLuint),
                    index_buffer, GL_STATIC_DRAW);
}

void Terrain::draw(const math::mat4x4 projection, const math::mat4x4 view,
                   const math::vec3 camera_pos) const noexcept {
    shader_program.use();
    glUniform1i(atlas_location, 1);
    glActiveTexture(GL_TEXTURE1);
    atlas.bind(GL_TEXTURE_2D);

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
