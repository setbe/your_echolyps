#include "terrain.hpp"

#include "block_list.hpp"

namespace hi {
void Terrain::generate_mesh_for(const Key &key, const Block *blocks,
                                std::vector<Vertex> &out) const noexcept {
    const unsigned W = Chunk::WIDTH, H = Chunk::HEIGHT, D = Chunk::DEPTH;

    std::unordered_map<Key, std::unique_ptr<Block[]>, Key::Hash> temp_neighbors;

    auto get_block = [&](int x, int y, int z) -> const Block * {
        return get_block_at_extended(key, blocks, x, y, z, temp_neighbors);
    };

    for (unsigned z = 0; z < D; ++z)
        for (unsigned y = 0; y < H; ++y)
            for (unsigned x = 0; x < W; ++x) {
                const Block &blk = blocks[z * W * H + y * W + x];
                if (blk.block_id() == BlockList::Air.block_id())
                    continue;

                const int gx = x + key.x * W, // x
                    gy = y + key.y * H,       // y
                    gz = z + key.z * D;       // z

                for (int face = 0; face < 6; ++face) {
                    const int dx = (face == 2) ? -1 : (face == 3) ? 1 : 0;
                    const int dy = (face == 5) ? -1 : (face == 4) ? 1 : 0;
                    const int dz = (face == 1) ? -1 : (face == 0) ? 1 : 0;
                    const Block *neighbour = get_block(x + dx, y + dy, z + dz);

                    const uint16_t neighbour_id = neighbour->block_id();
                    constexpr uint16_t air_id = BlockList::Air.block_id();
                    constexpr uint16_t water_id = BlockList::Water.block_id();
                    if (neighbour && (neighbour_id != air_id))
                        continue;

                    push_face(out, blk, gx, gy, gz, face);
                }
            }
}

const Block *Terrain::get_block_at_extended(
    const Key &center, const Block *blocks, int x, int y, int z,
    std::unordered_map<Key, std::unique_ptr<Block[]>, Key::Hash>
        &temp_neighbors) const noexcept {
    const int W = Chunk::WIDTH, H = Chunk::HEIGHT, D = Chunk::DEPTH;

    if (x >= 0 && x < W && y >= 0 && y < H && z >= 0 && z < D)
        return &blocks[z * W * H + y * W + x];

    int nx = x, ny = y, nz = z;
    Key nk = center;

    if (nx < 0) {
        nk.x--;
        nx += W;
    } else if (nx >= W) {
        nk.x++;
        nx -= W;
    }
    if (ny < 0) {
        nk.y--;
        ny += H;
    } else if (ny >= H) {
        nk.y++;
        ny -= H;
    }
    if (nz < 0) {
        nk.z--;
        nz += D;
    } else if (nz >= D) {
        nk.z++;
        nz -= D;
    }

    auto it = block_map.find(nk);
    const Block *neighbor = nullptr;

    if (it != block_map.end()) {
        neighbor = it->second.get();
    } else {
        auto temp_it = temp_neighbors.find(nk);
        if (temp_it == temp_neighbors.end()) {
            auto tmp = std::make_unique<Block[]>(Chunk::BLOCKS_PER_CHUNK);
            Chunk::generate_chunk(nk.x, nk.y, nk.z, tmp.get(), noise);
            neighbor = tmp.get();
            temp_neighbors.emplace(nk, std::move(tmp));
        } else {
            neighbor = temp_it->second.get();
        }
    }

    return &neighbor[nz * W * H + ny * W + nx];
}

void Terrain::push_face(std::vector<Vertex> &out, const Block &blk, int gx,
                        int gy, int gz, int face) const noexcept {
    using Tex = Block::TextureProtocol;

    constexpr int TPR = TEXTUREPACK_ATLAS_WIDTH / Tex::RESOLUTION;

    uint32_t bid = blk.block_id() - 1;
    uint32_t off = Tex::resolve_offset(blk.texture_protocol(), face);

    uint32_t tex = bid + off;
    uint32_t tx = tex % TPR, ty = tex / TPR;

    for (int i = 0; i < 6; ++i) {
        Vertex v;
        const float *POS = Block::CUBE_POS[face];
        v.position_block[0] = gx + POS[i * 3 + 0];
        v.position_block[1] = gy + POS[i * 3 + 1];
        v.position_block[2] = gz + POS[i * 3 + 2];
        v.position_block[3] = blk.to_float();
        v.uv[0] = (tx + Block::FACE_UVS[i * 2 + 0]) / float(TPR);
        v.uv[1] = (ty + Block::FACE_UVS[i * 2 + 1]) / float(TPR);
        out.push_back(v);
    }
}
} // namespace hi