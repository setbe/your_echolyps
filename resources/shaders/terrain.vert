#version 430 core

layout(std430, binding = 0) readonly buffer Vertices { float cube_vertices[]; };

layout(std430, binding = 1) readonly buffer BlockData {
    uint block_info[]; // packed: [id (16) | light (8) | faces (8)]
};

uniform mat4 projection;
uniform mat4 view;

const uint CHUNK_WIDTH = 16u;
const uint CHUNK_HEIGHT = 16u;
const uint CHUNK_DEPTH = 16u;

// Return position by index
uvec3 compute_block_position(uint instance_id) {
    uint x = instance_id % CHUNK_WIDTH;
    uint y = (instance_id / CHUNK_WIDTH) % CHUNK_HEIGHT;
    uint z = (instance_id / (CHUNK_WIDTH * CHUNK_HEIGHT)) % CHUNK_DEPTH;
    return uvec3(x, y, z);
}

bool is_face_visible(uint faces, uint direction) {
    return (faces & (1u << direction)) != 0u;
}

void main() {
    uint vertex_id = gl_VertexID;
    uint instance_id = gl_InstanceID;

    // Every triangle responsible for part of cube face
    uint face_index = vertex_id / 6u;

    // Extract packed float to uint
    uint packed_data = block_info[instance_id];
    uint id = packed_data & 0xFFFFu;
    uint light = (packed_data >> 16) & 0xFFu;
    uint faces = (packed_data >> 24) & 0xFFu;

    // Skip unvisible
    if (!is_face_visible(faces, face_index) || id == 0) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    // Extract cube vertex
    vec3 vertex =
        vec3(cube_vertices[vertex_id * 3 + 0], cube_vertices[vertex_id * 3 + 1],
             cube_vertices[vertex_id * 3 + 2]);

    // Calculate block position in the world
    vec3 offset = vec3(compute_block_position(instance_id));

    gl_Position = projection * view * vec4(vertex + offset, 1.0);
}
