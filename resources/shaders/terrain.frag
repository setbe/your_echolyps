#version 330 core
in vec2 frag_uv;
in float block_info;

uniform sampler2D atlas;
out vec4 out_color;

// float to uint32_t reinterpretation
uint floatBitsToUintSafe(float f) {
    return floatBitsToUint(f); // built-in in GLSL 3.30+
}

void main() {
    uint data = floatBitsToUint(block_info);

    // === ID fields ===
    // uint id = data & 0xFFFFu;
    // uint block_id = id & 0x0FFFu;        // lower 12 bits
    // uint tex_layout = (id >> 12) & 0xFu; // upper 4 bits

    // === Flags fields ===
    uint flags = (data >> 16) & 0xFFFFu;

    uint light = (flags >> 12) & 0xFu; // bits 15–12
    // uint visible_faces = (flags >> 6) & 0x3Fu; // bits 11–6

    float light_factor =
        max(0.2, float(light) / 15.0); // normalize to [0.0 .. 1.0]
    vec4 color = texture(atlas, frag_uv);
    out_color = vec4(color.rgb * light_factor, 1);
}
