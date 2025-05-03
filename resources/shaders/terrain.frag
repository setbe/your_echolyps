#version 330 core

in vec3 frag_normal;
in vec2 frag_uv;

out vec4 out_color;

void main() {
    vec3 light_dir = normalize(vec3(1.0, 1.0, 0.5));
    float light = max(dot(frag_normal, light_dir), 0.1) * 10;
    vec3 base_color = vec3(frag_uv, 1.0); // debug color using UV
    out_color = vec4(base_color, 1.0);
}
