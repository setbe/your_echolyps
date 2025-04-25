#pragma once

#include "external/glad.hpp"

#include "higui/higui.hpp"
#include "higui/higui_glyph.hpp"

#include "opengl.hpp"

using Handler = hi::window::Handler;

namespace hi {
struct Engine;
}

struct hi::Engine {
  private:
    static inline void noop(const hi::Callback &) noexcept {}
    static inline void noop_int(const hi::Callback &, int) noexcept {}
    static inline void noop_int_int(const hi::Callback &, int, int) noexcept {}

  public:
    hi::Callback callback;
    hi::Surface surface;
    hi::Opengl opengl;
    hi::TextRenderer text;

    inline explicit Engine(int width, int height) noexcept
        : callback{/* user_data */ this,
                   // -- Set noop functions ---
                   /* resize */ noop_int_int,
                   /* mouse_move */ noop_int_int,
                   /* key_up */ noop_int,
                   /* focus_gained */ noop,
                   /* focus_lost */ noop},
          surface{&callback, width, height}, opengl{} {

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