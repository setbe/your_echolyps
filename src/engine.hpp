#pragma once

#include "external/glad.hpp"

#include "higui/higui.hpp"

#include "opengl.hpp"

using Handler = hi::window::Handler;

namespace hi {
struct Engine {
  private:
    static inline void noop(const Callback &) noexcept {}
    static inline void noop_int(const Callback &, int) noexcept {}
    static inline void noop_int_int(const Callback &, int, int) noexcept {}

  public:
    Callback callback;
    Surface surface;
    Opengl opengl;

    inline explicit Engine(int width, int height, const char *title) noexcept
        : callback{/* user_data */ this,
                   // -- Set noop functions ---
                   /* resize */ noop_int_int,
                   /* mouse_move */ noop_int_int,
                   /* key_up */ noop_int,
                   /* focus_gained */ noop,
                   /* focus_lost */ noop},
          surface{&callback, width, height}, opengl{} {
        surface.set_title(title);
        callback.resize = framebuffer_resize_adapter;
        callback.focus_lost = focus_lost;
    }
    inline ~Engine() noexcept { /* RAII */ }

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine &operator=(Engine &&) = delete;

    void draw() const noexcept;

    void framebuffer_resize(const Callback &callback, int width,
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

} // namespace hi