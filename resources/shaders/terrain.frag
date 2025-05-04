#version 330 core
in vec2 frag_uv;
uniform sampler2D atlas;
out vec4 out_color;

void main() { out_color = texture(atlas, frag_uv); }
