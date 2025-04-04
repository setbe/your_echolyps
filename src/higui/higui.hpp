#pragma once

#if defined(__linux__)
    
#elif defined(_WIN32)
#include "higui_windows.hpp"
#elif defined(__APPLE__)
    
#elif defined(__ANDROID__)
    
#else
    
#endif


// ===== Contains all info related to crossplatform window management =====
namespace hi {
// ===== Surface: A lightweight abstraction over OS-dependent function calls =====
    struct Surface {
        inline explicit Surface(int width, int height) noexcept
            : handler_(window::create(width, height)) { }

        inline ~Surface() noexcept { window::destroy(handler_); }

        Surface() = delete;
        Surface(const Surface&) = delete;
        Surface& operator=(const Surface&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface&&) = delete;

        void loop() const noexcept { window::loop(handler_); }

        inline bool is_handler() const noexcept { 
#ifdef __linux__
            return handler_ != 0;
#elif defined(_WIN32)
            return handler_ != nullptr;
#endif
        }

        inline void set_title(const char* title) const noexcept {
            window::set_title(handler_, title);
        }

    private:
        window::Handler handler_;
    }; // struct Surface

} // namespace hi