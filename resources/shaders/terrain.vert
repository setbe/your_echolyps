#version 330 core

layout(location = 0) in vec4 in_position_block;

uniform mat4 projection;
uniform mat4 view;

flat out uint block_id;
flat out uint block_flags;

void main() {
    vec3 pos = in_position_block.xyz;
    uint blk = floatBitsToUint(in_position_block.w);

    block_id = blk & 0xFFFFu;
    block_flags = (blk >> 16);

    gl_Position = projection * view * vec4(pos, 1.0);
}
