#pragma once
#include "../external/glad.hpp"
#include "../fonts.hpp"
#include "../opengl.hpp"
#include "higui_debug.hpp"

#include <stdint.h>

namespace hi {
struct TextRenderer {
    unsigned vao = 0;
    unsigned vbo = 0;
    unsigned ebo = 0;
    unsigned shader_program = 0;
    unsigned texture = 0;
    int text_atlas_location = 0;

    static constexpr int max_chars = 256;
    static constexpr int floats_per_vertex = 4; // x, y, u, v
    static constexpr int verts_per_char = 4;
    static constexpr int inds_per_char = 6;
    static constexpr int max_vertices = max_chars * verts_per_char;
    static constexpr int max_indices = max_chars * inds_per_char;

    float vertices[max_vertices * floats_per_vertex]{};
    unsigned indices[max_indices]{};

    int char_count = 0;

    explicit TextRenderer() noexcept {}
    inline ~TextRenderer() noexcept {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &texture);
        glDeleteProgram(shader_program);
    }

    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;
    TextRenderer(TextRenderer &&) = delete;
    TextRenderer &operator=(TextRenderer &&) = delete;

    inline uint32_t decode_utf8(const char *&ptr) noexcept {
        uint8_t c = (uint8_t)*ptr++;
        if (c < 0x80)
            return c;
        if ((c >> 5) == 0x6) {
            uint32_t c2 = (uint8_t)*ptr++;
            return ((c & 0x1F) << 6) | (c2 & 0x3F);
        }
        if ((c >> 4) == 0xE) {
            uint32_t c2 = (uint8_t)*ptr++;
            uint32_t c3 = (uint8_t)*ptr++;
            return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        }
        if ((c >> 3) == 0x1E) {
            uint32_t c2 = (uint8_t)*ptr++;
            uint32_t c3 = (uint8_t)*ptr++;
            uint32_t c4 = (uint8_t)*ptr++;
            return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) |
                   ((c3 & 0x3F) << 6) | (c4 & 0x3F);
        }
        return 0xFFFD; // replacement character
    }

    inline void init(const unsigned char *pixels,
                     const char *sampler_name = "textAtlas") noexcept {
        // Initialize fields
        shader_program =
            create_shader_program(text_shader_vert, text_shader_frag);
        text_atlas_location =
            glGetUniformLocation(shader_program, sampler_name);

        // Create texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_ATLAS_WIDTH,
                     FONT_ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Buffers
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), nullptr,
                     GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), nullptr,
                     GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void *)(2 * sizeof(float)));
    }

    void add_text(const char *text, float x, float y,
                  float scale = 1.0f) noexcept {
        char_count = 0;
        float pen_x = x;
        float pen_y = y;

        const char *ptr = text;
        while (*ptr && char_count < max_chars) {
            int cp = decode_utf8(ptr);
            const GlyphInfo *glyph = nullptr;
            for (const auto &g : font_glyphs) {
                if (g.codepoint == cp) {
                    glyph = &g;
                    break;
                }
            }
            if (!glyph)
                continue;

            float xpos = pen_x + glyph->offset_x * scale;
            float ypos = pen_y + FONT_ASCENT * scale;
            float w = glyph->w * scale;
            float h = glyph->h * scale;

            float u1 = glyph->x / float(FONT_ATLAS_WIDTH);
            float v1 = glyph->y / float(FONT_ATLAS_HEIGHT);
            float u2 = (glyph->x + glyph->w) / float(FONT_ATLAS_WIDTH);
            float v2 = (glyph->y + glyph->h) / float(FONT_ATLAS_HEIGHT);

            int v_offset = char_count * verts_per_char * floats_per_vertex;
            int i_offset = char_count * inds_per_char;
            int vi = char_count * verts_per_char;

            float *v = &vertices[v_offset];
            v[0] = xpos;
            v[1] = ypos;
            v[2] = u1;
            v[3] = v1;
            v[4] = xpos + w;
            v[5] = ypos;
            v[6] = u2;
            v[7] = v1;
            v[8] = xpos + w;
            v[9] = ypos + h;
            v[10] = u2;
            v[11] = v2;
            v[12] = xpos;
            v[13] = ypos + h;
            v[14] = u1;
            v[15] = v2;

            unsigned *id = &indices[i_offset];
            id[0] = vi + 0;
            id[1] = vi + 1;
            id[2] = vi + 2;
            id[3] = vi + 2;
            id[4] = vi + 3;
            id[5] = vi + 0;

            pen_x += glyph->advance * scale;
            char_count++;
        }
    }

    inline void upload() const noexcept {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        char_count * verts_per_char * floats_per_vertex *
                            sizeof(float),
                        vertices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                        char_count * inds_per_char * sizeof(unsigned), indices);
    }

    inline void draw() const noexcept {
        glUseProgram(shader_program);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(text_atlas_location, 0);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, char_count * inds_per_char,
                       GL_UNSIGNED_INT, nullptr);
    }
};

} // namespace hi
