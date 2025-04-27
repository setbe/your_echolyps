#pragma once

#include "external/glad.hpp"

#include "fonts.hpp"
#include "higui/higui.hpp"
#include "higui/higui_glyph.hpp"

#include "opengl.hpp"

using Handler = hi::window::Handler;

namespace hi {
struct Engine;
}

struct hi::Engine {
    struct Font {
        unsigned char *font_bitmap;
        constexpr static unsigned font_memory_size =
            FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT;

        inline explicit Font() noexcept
            : font_bitmap{
                  static_cast<unsigned char *>(hi::alloc(font_memory_size))} {

            if (!font_bitmap)
                panic(Result{Stage::Engine, Error::FontMemoryAlloc});

            decompress_font_bitmap(font_bitmap);
        }

        inline ~Font() noexcept { hi::free(font_bitmap, font_memory_size); }

        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;
        Font(Font &&) = delete;
        Font &operator=(Font &&) = delete;
    };

  private:
    static inline void noop(const hi::Callback &) noexcept {}
    static inline void noop_int(const hi::Callback &, int) noexcept {}
    static inline void noop_int_int(const hi::Callback &, int, int) noexcept {}

  public:
    hi::Callback callback;
    hi::Surface surface;
    hi::Opengl opengl;
    hi::TextRenderer text;
    Font font;

    inline explicit Engine(int width, int height) noexcept
        : callback{/* user_data */ this,
                   // -- Set noop functions ---
                   /* resize */ noop_int_int,
                   /* mouse_move */ noop_int_int,
                   /* key_up */ key_up,
                   /* focus_gained */ noop,
                   /* focus_lost */ noop},
          surface{&callback, width, height}, opengl{}, font{} {

        callback.resize = framebuffer_resize_adapter;
        callback.focus_lost = focus_lost;

        start();
    }
    inline ~Engine() noexcept { /* RAII */ }

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine &operator=(Engine &&) = delete;

    void start() noexcept;
    void draw() const noexcept;

    static void key_up(const Callback &cb, key::KeyCode key) noexcept;

    inline void framebuffer_resize(const Callback &callback, int width,
                                   int height) const noexcept {
        opengl.framebuffer_resize(callback, width, height);
        this->draw();
        hi::sleep(7); // hack
    }

    static void framebuffer_resize_adapter(const Callback &cb, int w,
                                           int h) noexcept {
        const Engine *engine = cb.get_user_data<Engine>();
        engine->framebuffer_resize(cb, w, h);
    }

    static void focus_lost(const Callback &cb) noexcept {
        hi::trim_working_set();
    }
};