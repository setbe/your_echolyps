#pragma once

#include "../external/glad.hpp"
#include "../world/world.hpp"

#include "../higui/glyph.hpp"
#include "../higui/higui.hpp"

#include "../resources/fonts.hpp"
#include "opengl.hpp"

namespace hi {
struct Engine;

struct Font {
    unsigned char *font_bitmap;
    constexpr static unsigned font_memory_size = FONT_ATLAS_W * FONT_ATLAS_H;

    inline explicit Font() noexcept
        : font_bitmap{
              static_cast<unsigned char *>(hi::alloc(font_memory_size))} {

        if (!font_bitmap)
            panic(Result{Stage::Engine, Error::FontMemoryAlloc});

        if (!decompress_font_bitmap(font_bitmap))
            panic(Result{Stage::Engine, Error::FontBitmapDecompress});
    }

    inline ~Font() noexcept { hi::free(font_bitmap, font_memory_size); }

    Font(const Font &) = delete;
    Font &operator=(const Font &) = delete;
    Font(Font &&) = delete;
    Font &operator=(Font &&) = delete;
}; // struct Font

struct OpenglLoader {
    inline explicit OpenglLoader() noexcept { gl::load(); }
    inline ~OpenglLoader() noexcept {}
    OpenglLoader(const OpenglLoader &) = delete;
    OpenglLoader &operator=(const OpenglLoader &) = delete;
    OpenglLoader(OpenglLoader &&) = delete;
    OpenglLoader &operator=(OpenglLoader &&) = delete;
}; // struct OpenglLoader

} // namespace hi

// ===== Engine: Connects all shit together
// =====
struct hi::Engine {
    struct Config {
        int width{};
        int height{};
        bool is_wireframe;
        bool is_cursor;
        bool is_fullscreen;

        constexpr inline Config() noexcept
            : is_wireframe{false}, is_cursor{false}, is_fullscreen{false} {}
    }; // struct Config

  private:
    static inline void noop(const Callback &) noexcept {}
    static inline void noop_int(const Callback &, int) noexcept {}
    static inline void noop_int_int(const Callback &, int, int) noexcept {}

  public:
    Callback callback;
    Surface surface;
    OpenglLoader gl_loader;
    TextRenderer text;
    World world;
    Font font;
    Config config;

    inline explicit Engine(int width, int height) noexcept
        : callback{/* user_data */ this,
                   // -- Set noop functions ---
                   /* resize */ noop_int_int,
                   /* mouse_move */ noop_int_int,
                   /* key_up */ key_up,
                   /* focus_gained */ noop,
                   /* focus_lost */ noop},
          surface{&callback, width, height}, gl_loader{}, world{}, font{},
          config{} {

        callback.resize = framebuffer_resize_adapter;
        callback.mouse_move = mouse_move_adapter;
        callback.focus_lost = focus_lost;
        callback.key_up = key_up;
        start();
    }
    inline ~Engine() noexcept { /* RAII */ }

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine &operator=(Engine &&) = delete;

    void start() noexcept;
    void update() noexcept;
    void draw() const noexcept;

    static void key_up(const Callback &cb, key::KeyCode key) noexcept;

    inline void framebuffer_resize(const Callback &callback, int width,
                                   int height) noexcept {
        config.width = width;
        config.height = height;
        gl::framebuffer_resize(callback, width, height);
        world.update_projection(width, height);
        this->draw();
        hi::sleep(7); // hack
    }

    static void framebuffer_resize_adapter(const Callback &cb, int w,
                                           int h) noexcept {
        Engine *engine = cb.get_user_data<Engine>();
        engine->framebuffer_resize(cb, w, h);
    }

    static void focus_lost(const Callback &cb) noexcept {
        hi::trim_working_set();
    }

    inline void mouse_move(int x, int y) noexcept {
        if (!config.is_cursor)
            return;

        constexpr unsigned padding = 50;
        static int last_x = x;
        static int last_y = y;

        if (x < padding || y < padding || x > config.width - padding ||
            y > config.height - padding) {
            surface.center_cursor();
            last_x = config.width / 2;
            last_y = config.height / 2;
            return;
        }

        int xoffset = x - last_x;
        int yoffset = last_y - y;
        last_x = x;
        last_y = y;

        world.camera_rotate(xoffset, yoffset);
    }

    static void mouse_move_adapter(const Callback &cb, int x, int y) noexcept {
        Engine *engine = cb.get_user_data<Engine>();
        engine->mouse_move(x, y);
    }
};