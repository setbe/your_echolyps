#pragma once

#ifndef HIGUI_GLYPH_USE_STATIC_DRAW
#define HIGUI_GLYPH_DRAW_USAGE GL_DYNAMIC_DRAW
#else
#define HIGUI_GLYPH_DRAW_USAGE GL_STATIC_DRAW
#endif

#include "../engine/opengl.hpp"
#include "../resources/fonts.hpp"
#include "../resources/shaders.hpp"
#include "types.hpp"

#include <stdarg.h>
#include <stddef.h>

namespace hi {
struct TextRenderer {
    GlyphInfo *font_glyphs;
    static constexpr unsigned FONT_GLYPHS_SIZE = sizeof(GlyphInfo) * FONT_COUNT;

    gl::VAO vao;
    gl::VBO vbo;
    gl::EBO ebo;
    gl::ShaderProgram shader_program;
    gl::Texture text_atlas;
    int text_atlas_location = 0;

    static constexpr int FONT_ASCENT = 16;
    static constexpr int MAX_CHARS = 256;
    static constexpr int FLOATS_PER_VERTEX = 4; // x, y, u, v
    static constexpr int VERTS_PER_CHAR = 4;
    static constexpr int INDS_PER_CHAR = 6;
    static constexpr int MAX_VERTICES = MAX_CHARS * VERTS_PER_CHAR;
    static constexpr int MAX_INDICES = MAX_CHARS * INDS_PER_CHAR;

    float vertices[MAX_VERTICES * FLOATS_PER_VERTEX]{};
    unsigned indices[MAX_INDICES]{};

    int char_count = 0;

    inline explicit TextRenderer() noexcept
        : vao{}, vbo{}, ebo{},
          shader_program{text_shader_vert, text_shader_frag}, text_atlas{} {

        font_glyphs = static_cast<GlyphInfo *>(hi::alloc(FONT_GLYPHS_SIZE));
        if (!font_glyphs)
            panic(Result{Stage::Engine, Error::FontGlyphsMemoryAlloc});

        if (!decompress_font_glyphs(font_glyphs))
            panic(Result{Stage::Engine, Error::FontDecompressMemoryAlloc});
    }

    inline ~TextRenderer() noexcept { hi::free(font_glyphs, FONT_GLYPHS_SIZE); }

    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;
    TextRenderer(TextRenderer &&) = delete;
    TextRenderer &operator=(TextRenderer &&) = delete;

    void add_text(float x, float y, float scale, const char *fmt, ...) noexcept;

    static inline uint32_t decode_utf8(const char *&ptr) noexcept {
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
        return 0xFFFD;
    }

    inline void init(const unsigned char *pixels,
                     const char *sampler_name = "textAtlas") noexcept {
        text_atlas_location =
            glGetUniformLocation(shader_program.get(), sampler_name);

        glBindTexture(GL_TEXTURE_2D, text_atlas.get());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_ATLAS_W, FONT_ATLAS_H, 0,
                     GL_RED, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        vao.bind();

        vbo.bind(GL_ARRAY_BUFFER);
        vbo.buffer_data(GL_ARRAY_BUFFER, sizeof(vertices), nullptr,
                        HIGUI_GLYPH_DRAW_USAGE);

        ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
        ebo.buffer_data(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), nullptr,
                        HIGUI_GLYPH_DRAW_USAGE);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void *)(2 * sizeof(float)));
    }

    inline void upload() const noexcept {
        vao.bind();
        vbo.bind(GL_ARRAY_BUFFER);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        char_count * VERTS_PER_CHAR * FLOATS_PER_VERTEX *
                            sizeof(float),
                        vertices);
        ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                        char_count * INDS_PER_CHAR * sizeof(unsigned), indices);
    }

    inline void draw() const noexcept {
        shader_program.use();
        text_atlas.bind(GL_TEXTURE_2D);
        glUniform1i(text_atlas_location, 0);

        vao.bind();
        glDrawElements(GL_TRIANGLES, char_count * INDS_PER_CHAR,
                       GL_UNSIGNED_INT, nullptr);
    }
};
} // namespace hi
