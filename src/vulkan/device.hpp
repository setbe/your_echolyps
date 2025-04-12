#pragma once

#include <vulkan/vulkan.h>

#include "../higui/higui.hpp"
#include "../higui/higui_types.hpp"
#include "../higui/higui_debug.hpp"

#ifndef NDEBUG
constexpr bool vk_enable_validation_layers = true;
#else
constexpr bool vk_enable_validation_layers = false;
#endif

static constexpr const char* vk_required_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifndef NDEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
#ifdef _WIN32
        "VK_KHR_win32_surface",
#endif
};

namespace hi {
    // --- structs for swapchain support and queue
    struct SwapChainSupportDetails {
        using SurfaceFormat = VkSurfaceFormatKHR[16];
        using PresentMode = VkPresentModeKHR[8];
        VkSurfaceCapabilitiesKHR capabilities;
        SurfaceFormat formats;
        uint32_t format_count;
        VkPresentModeKHR present_modes[8];
        uint32_t present_mode_count;
    };

    struct QueueFamilyIndices {
        uint32_t graphics_family;
        uint32_t present_family;
        bool graphics_family_has_value = false;
        bool present_family_has_value = false;
        bool is_complete() { return graphics_family_has_value && present_family_has_value; }
    };

    // use `.init()`
    class EngineDevice {
    public:
        inline explicit EngineDevice(hi::Surface& window) noexcept
            : window_(window) {}
        ~EngineDevice() noexcept;

        EngineDevice(const EngineDevice&) = delete;
        EngineDevice& operator=(const EngineDevice&) = delete;
        EngineDevice(EngineDevice&&) = delete;
        EngineDevice& operator=(EngineDevice&&) = delete;

        inline Result init() noexcept {
            StageError stage;
            Error code;
            HI_STAGE_CHECK(CreateInstance, create_instance)
#ifndef NDEBUG
                HI_STAGE_CHECK(SetupDebugMessenger, setup_debug_messenger)
#endif
                HI_STAGE_CHECK(CreateSurface, create_surface)
                HI_STAGE_CHECK(PickPhysicalDevice, pick_physical_device)
                HI_STAGE_CHECK(CreateLogicalDevice, create_logical_device)
                HI_STAGE_CHECK(CreateCommandPool, create_command_pool)
                return { stage, code };
        }

        VkCommandPool command_pool() { return this->command_pool_; }
        VkDevice device() { return this->device_; }
        VkSurfaceKHR surface() { return this->surface_; }
        VkQueue graphics_queue() { return this->graphics_queue_; }
        VkQueue present_queue() { return this->present_queue_; }

        SwapChainSupportDetails get_swapchain_support() { return query_swapchain_support(this->physical_device_); }
        uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) noexcept;
        QueueFamilyIndices find_physical_queue_families() { return find_queue_families(this->physical_device_); }
        VkFormat find_supported_format(Error& error, const VkFormat* candidates,
            uint32_t count,
            VkImageTiling tiling,
            VkFormatFeatureFlags features) noexcept;

        /* === Buffer functions === */

        hi::Error create_buffer(VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& buffer_memory) noexcept;
        VkCommandBuffer begin_single_time_commands() noexcept;
        void end_single_time_commands(VkCommandBuffer command_buffer) noexcept;
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) noexcept;
        void copy_buffer_to_image(VkBuffer buffer,
            VkImage image,
            uint32_t width,
            uint32_t height,
            uint32_t layer_count) noexcept;
        hi::Error create_image_with_info(const VkImageCreateInfo& image_info,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& image_memory) noexcept;

        VkPhysicalDeviceProperties properties_;

    private:
        hi::Error create_instance();
        hi::Error setup_debug_messenger();
        hi::Error create_surface();
        hi::Error pick_physical_device();
        hi::Error create_logical_device();
        hi::Error create_command_pool();

        /* === Helper functions === */

        bool is_device_suitable(VkPhysicalDevice device);

#ifndef NDEBUG
        bool check_validation_layer_support();
#endif

        QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

        bool check_device_extension_support(VkPhysicalDevice device);
        SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device);

        /* === Variables === */

        VkInstance instance_;

#ifndef NDEBUG
        VkDebugUtilsMessengerEXT debug_messenger_;
#endif

        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
        hi::Surface& window_;
        VkCommandPool command_pool_;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphics_queue_;
        VkQueue present_queue_;

#ifndef NDEBUG
        static constexpr const char* validation_layers_[1] = {
            "VK_LAYER_KHRONOS_validation"
        };
#endif

        static constexpr const char* device_extensions_[1] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };
} // namespace hi
