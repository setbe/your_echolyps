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
        inline explicit Surface(int width, int height) noexcept
            : handler_{ window::create(width, height) } {}

        inline explicit Surface(hi::window::Handler handler) noexcept
            : handler_{ handler } {}

        inline ~Surface() noexcept {
            window::destroy(handler_); 
        }

        Surface() = delete;
        Surface(const Surface&) = delete;
        Surface& operator=(const Surface&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface&&) = delete;

        inline void loop() const noexcept {
            window::loop(handler_); 
        }
        
        inline bool is_handler() const noexcept { return window::is_valid(handler_); }
        inline window::Handler get_handler() const noexcept { return handler_; }
        
        inline void set_title(const char* title) const noexcept { 
            window::set_title(handler_, title); 
        }
        
        inline VkResult create_vulkan_surface(VkInstance instance, VkSurfaceKHR* surface) const noexcept {
            return window::create_vulkan_surface(handler_, instance, surface); 
        }
    }; // struct Surface

} // namespace hi