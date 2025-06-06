#version 330 core

layout(location = 0) in vec4 in_pos_block;
layout(location = 1) in vec2 in_uv;

uniform mat4 projection;
uniform mat4 view;

out vec2 frag_uv;
out float block_info;

void main() {
    frag_uv = in_uv;
    block_info = in_pos_block.w; // transfer .w as float
    gl_Position = projection * view * vec4(in_pos_block.xyz, 1.0);
}
