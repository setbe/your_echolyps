#pragma once
#include "higui_platform.hpp"

// ===== Contains all info related to crossplatform window management =====
namespace hi {
    // ===== Surface: A lightweight abstraction over OS-dependent function calls =====
    // use `.init()`
    struct Surface {
    private:
        window::Handler handler_{};
    public:
        inline explicit Surface() noexcept {};

        inline Error init(int width, int height, const Callback* callback) noexcept {
            Error error = Error::None;
            handler_ = window::create(width, height, error, callback);
            if (error != Error::None)
                return error;
            return window::setup_opengl_context(handler_);
        }

        inline explicit Surface(hi::window::Handler handler) noexcept
            : handler_{ handler } {}

        inline ~Surface() noexcept {
            window::destroy(handler_); 
        }

        Surface(const Surface&) = delete;
        Surface& operator=(const Surface&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface&&) = delete;

        inline bool poll_events() const noexcept {
            return window::poll_events(handler_);
        }
        
        inline bool is_handler() const noexcept { return window::is_valid(handler_); }
        inline window::Handler get_handler() const noexcept { return handler_; }
        
        inline void set_title(const char* title) const noexcept { 
            window::set_title(handler_, title); 
        }
    }; // struct Surface

} // namespace hi