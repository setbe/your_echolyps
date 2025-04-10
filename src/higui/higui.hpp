#pragma once

#include <vulkan/vulkan.h>

#include "higui_platform.hpp"
#include "higui_event.hpp"

// ===== Contains all info related to crossplatform window management =====
namespace hi {
    // ===== Surface: A lightweight abstraction over OS-dependent function calls =====
    struct Surface {
    private:
        window::Handler handler_;
    public:
        explicit Surface(int width, int height) noexcept
            : handler_(window::create(width, height)) {} 

        explicit Surface(hi::window::Handler handler) noexcept
            : handler_(handler) {}

        ~Surface() noexcept { 
            window::destroy(handler_); 
        }

        Surface() = delete;
        Surface(const Surface&) = delete;
        Surface& operator=(const Surface&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface&&) = delete;

        void loop() const noexcept { 
            window::loop(handler_); 
        }
        
        bool is_handler() const noexcept { return window::is_valid(handler_); }
        window::Handler get_handler() const noexcept { return handler_; }
        
        void set_title(const char* title) const noexcept { 
            window::set_title(handler_, title); 
        }
        
        VkResult create_vulkan_surface(VkInstance instance, VkSurfaceKHR* surface) const noexcept { 
            return window::create_vulkan_surface(handler_, instance, surface); 
        }
    }; // struct Surface

} // namespace hi