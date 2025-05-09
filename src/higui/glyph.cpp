#include "glyph.hpp"

namespace hi {

static inline int vsnprintf(char *str, size_t size, const char *format,
                            va_list ap) {
    size_t len = 0;
    const char *p = format;
    static char buf[64]{};
    static char tmp[32]{};

    while (*p && len < size - 1) {
        if (*p == '%') {
            p++;
            switch (*p) {
            case 's': {
                const char *s = va_arg(ap, const char *);
                while (*s && len < size - 1)
                    str[len++] = *s++;
                break;
            }
            case 'd': {
                int d = va_arg(ap, int);
                int i = 0;
                if (d < 0) {
                    str[len++] = '-';
                    d = -d;
                }
                do {
                    tmp[i++] = '0' + (d % 10);
                    d /= 10;
                } while (d && i < 31);
                while (i-- && len < size - 1)
                    str[len++] = tmp[i];

                for (char *s = buf; *s && len < size - 1; ++s)
                    str[len++] = *s;
                break;
            }
            case 'f': {
                double f = va_arg(ap, double);
                if (f < 0) {
                    if (len < size - 1)
                        str[len++] = '-';
                    f = -f;
                }

                int int_part = (int)f;
                double frac_part = f - int_part;
                int i = 0;
                if (int_part == 0) {
                    tmp[i++] = '0';
                } else {
                    while (int_part && i < 31) {
                        tmp[i++] = '0' + (int_part % 10);
                        int_part /= 10;
                    }
                }
                while (i-- && len < size - 1)
                    str[len++] = tmp[i];

                if (len < size - 1)
                    str[len++] = '.';

                frac_part *= 1000000.0;
                int frac = (int)(frac_part + 0.5);

                for (int div = 100000; div > 0 && len < size - 1; div /= 10) {
                    str[len++] = '0' + (frac / div % 10);
                }

                break;
            }

            case '%': {
                str[len++] = '%';
                break;
            }
            default:
                break;
            }
        } else {
            str[len++] = *p;
        }
        p++;
    }

    str[len] = '\0';
    return (int)len;
}

extern "C" inline const char *vformat(const char *fmt, va_list args) {
    static char buffer[256]; // Not thread-safe. Ensure to change `static` to
                             // `thread_local`
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    return buffer;
}

void TextRenderer::add_text(float x, float y, float scale, const char *fmt,
                            ...) noexcept {
    char_count = 0;
    float pen_x = x;
    float pen_y = y;

    va_list args;
    va_start(args, fmt);
    const char *text = vformat(fmt, args);
    va_end(args);

    const char *ptr = text;
    while (*ptr && char_count < MAX_CHARS) {
        int cp = decode_utf8(ptr);

        if (cp == '\n') {
            pen_x = x;
            pen_y -= FONT_ASCENT * scale;
            continue;
        }

        const GlyphInfo *glyph = nullptr;
        for (unsigned i = 0; i < FONT_COUNT; ++i) {
            if (font_glyphs[i].codepoint == cp) {
                glyph = &font_glyphs[i];
                break;
            }
        }
        if (!glyph)
            continue;

        float xpos = pen_x + glyph->offset_x * scale;
        float ypos = pen_y + glyph->advance * scale;
        float w = glyph->w * scale / 1.5f;
        float h = glyph->h * scale;

        float u1 = glyph->x / float(FONT_ATLAS_W);
        float v1 = glyph->y / float(FONT_ATLAS_H);
        float u2 = (glyph->x + glyph->w) / float(FONT_ATLAS_W);
        float v2 = (glyph->y + glyph->h) / float(FONT_ATLAS_H);

        int v_offset = char_count * VERTS_PER_CHAR * FLOATS_PER_VERTEX;
        int i_offset = char_count * INDS_PER_CHAR;
        int vi = char_count * VERTS_PER_CHAR;

        float *vertex = &vertices[v_offset];
        vertex[0] = xpos;
        vertex[1] = ypos;
        vertex[2] = u1;
        vertex[3] = v1;
        vertex[4] = xpos + w;
        vertex[5] = ypos;
        vertex[6] = u2;
        vertex[7] = v1;
        vertex[8] = xpos + w;
        vertex[9] = ypos + h;
        vertex[10] = u2;
        vertex[11] = v2;
        vertex[12] = xpos;
        vertex[13] = ypos + h;
        vertex[14] = u1;
        vertex[15] = v2;

        unsigned *id = &indices[i_offset];
        id[0] = vi + 0;
        id[1] = vi + 1;
        id[2] = vi + 2;
        id[3] = vi + 2;
        id[4] = vi + 3;
        id[5] = vi + 0;

        pen_x += glyph->advance * scale / 1.5f;
        char_count++;
    }
}
} // namespace hi
