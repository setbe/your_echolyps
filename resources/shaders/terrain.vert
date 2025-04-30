#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

uniform mat4 projection;
uniform mat4 view;

out vec3 frag_normal;
out vec2 frag_uv;

void main() {
    frag_normal = in_normal;
    frag_uv = in_uv;
    gl_Position = projection * view * vec4(in_position, 1.0);
}
