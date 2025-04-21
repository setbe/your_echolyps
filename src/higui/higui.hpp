#pragma once
#include "higui_platform.hpp"

namespace hi {
    // ===== Surface: A lightweight abstraction over OS-dependent function calls =====
    struct Surface {
    private:
        window::Handler handler_;
        window::GraphicsContext grapchics_context_;
    public:
        inline explicit Surface(const Callback* callback, int width, int height) noexcept
            :
            handler_{ window::create(callback, grapchics_context_, width, height) } {
            window::setup_opengl_context(handler_, grapchics_context_);
        }

        inline ~Surface() noexcept { window::destroy(handler_); }

        Surface(const Surface&) = delete;
        Surface& operator=(const Surface&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface&&) = delete;

        inline bool poll_events() const noexcept { return window::poll_events(handler_); }
        inline bool is_handler() const noexcept { return window::is_valid(handler_); }
        inline const window::Handler get_handler() const noexcept { return handler_; }
        inline const window::GraphicsContext get_graphics_context() const noexcept { return grapchics_context_; }
        inline void set_title(const char* title) const noexcept { window::set_title(handler_, title); }
        inline void swap_buffers() const noexcept { window::swap_buffers(get_graphics_context()); }
    }; // struct Surface

} // namespace hi