#pragma once

#include <vulkan/vulkan.h>

#include "higui_platform.hpp"
#include "higui_event.hpp"

// ===== Contains all info related to crossplatform window management =====
namespace hi {
    // ===== Surface: A lightweight abstraction over OS-dependent function calls =====
    // use `.init()`
    struct Surface {
    private:
        window::Handler handler_;
    public:
        inline explicit Surface() noexcept {};

        inline Result init(int width, int height) noexcept {
            Result result{
                .stage_error = hi::StageError::CreateWindow,
                .error_code = hi::Error::None
            };
            handler_ = window::create(width, height, result);
            return result;
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
        
        inline VkResult create_vulkan_surface(VkInstance instance, VkSurfaceKHR* surface) const noexcept {
            return window::create_vulkan_surface(handler_, instance, surface); 
        }

        inline VkExtent2D get_extent() const noexcept {
            int width, height;
            window::get_size(handler_, width, height);
            return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        }
    }; // struct Surface

} // namespace hi