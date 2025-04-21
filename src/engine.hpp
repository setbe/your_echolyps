#pragma once

#include "external/glad.hpp"

#include "higui/higui.hpp"

#include "opengl.hpp"

using Handler = hi::window::Handler;

namespace hi {
    struct Engine {
        Callback callback;
        Surface surface;

        inline explicit Engine(int width, int height, const char* title) noexcept
            :
            callback{ this }, 
            surface { &callback, width, height } {

#if defined(_WIN32) && !defined(HI_MULTITHREADING_USED)
            static bool first_time = true;
#else // use multithreading
            thread_local bool first_time = true;
#endif // _WIN32 && HI_MULTITHREADING_USED
            
            if (first_time) {
                window::load_gl();
                first_time = false;
            }

            surface.set_title(title);
            callback.resize = framebuffer_resize_adapter;
            callback.focus_lost = focus_lost;
            opengl::create_shader_program(default_vert, default_frag);
            opengl::bind_buffers();
        }
        inline ~Engine() noexcept {}

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        void draw() const noexcept {
            opengl::draw();
            surface.swap_buffers();
        }

        void framebuffer_resize(const Callback& callback, int width, int height) const noexcept { 
            opengl::framebuffer_resize(callback, width, height);
            this->draw();
            hi::sleep(7); // hack
        }
        
        static void framebuffer_resize_adapter(const Callback& cb, int w, int h) noexcept {
            Engine* engine = cb.get_user_data<Engine>();
            engine->framebuffer_resize(cb, w, h);
        }
        
        static void focus_lost(const Callback& cb) noexcept {
            hi::trim_working_set();
        }

        inline void run() const noexcept {
            while (surface.poll_events())
            {
                this->draw();
            }
        }        
    };

} // namespace hi