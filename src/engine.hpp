#pragma once
#include "higui/higui.hpp"
#include "external/glad.h"

using Handler = hi::window::Handler;
constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;

#include <stdio.h>

namespace hi {
    // use `.init()`
    struct Engine {
        Callback callback;
        Surface surface;

        inline explicit Engine() noexcept
            : callback{ this }, 
            surface {}
        {}
 
        inline ~Engine() noexcept {
        }

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        void draw() const noexcept {
            glClearColor(0.2f, 0.5f, 0.8f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT);

            window::swap_buffers(surface.get_handler());
        }

        // zero == success
        int init() noexcept;

        void framebuffer_resize(const Callback& callback, int width, int height) const noexcept {
            glViewport(0, 0, width, height);
        }

        static void framebuffer_resize_adapter(const Callback& cb, int w, int h) noexcept {
            Engine* engine = cb.get_user_data<Engine>();
            engine->framebuffer_resize(cb, w, h);
        }

        inline void run() const noexcept {
            trim_working_set();
            while (surface.poll_events())
            {
                draw();
            }
        }        
    };

} // namespace hi