#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include "../higui/higui_types.hpp"
#include "device.hpp"

namespace hi {

    // use `.init()`
    struct SwapChain {
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SwapChain(EngineDevice& device_ref, VkExtent2D extent) noexcept
            : device_{ device_ref }, window_extent_{ extent } {}
        ~SwapChain() noexcept;

        SwapChain() = delete;
        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain(SwapChain&&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        inline Result init() noexcept {
            StageError stage;
            Error code;
            HI_STAGE_CHECK(CreateSwapChain, create_swapchain);
            HI_STAGE_CHECK(CreateImageViews, create_image_views);
            HI_STAGE_CHECK(CreateRenderPass, create_render_pass);
            HI_STAGE_CHECK(CreateDepthResources, create_depth_resources);
            HI_STAGE_CHECK(CreateFramebuffers, create_framebuffers);
            HI_STAGE_CHECK(CreateSyncObjects, create_sync_objects);
            return Result{ .stage_error = stage, .error_code = code };
        }

        inline VkFramebuffer framebuffer(int index) const noexcept { return framebuffers_[index]; }
        inline VkRenderPass render_pass() const noexcept { return render_pass_; }
        inline VkImageView image_view(int index) const noexcept { return image_views_[index]; }
        inline size_t image_count() const noexcept { return images_.size(); }
        inline VkFormat image_format() const noexcept { return image_format_; }
        inline VkExtent2D extent() const noexcept { return extent_; }
        inline uint32_t width() const noexcept { return extent_.width; }
        inline uint32_t height() const noexcept { return extent_.height; }
        inline float aspect_ratio() const noexcept { return static_cast<float>(extent_.width) / static_cast<float>(extent_.height); }

        VkFormat find_depth_format(Error& error) const noexcept;
        VkResult acquire_next_image(uint32_t* image_index) const noexcept;
        VkResult submit_command_buffers(const VkCommandBuffer* buffers, uint32_t* image_index) noexcept;

    private:
        Error create_swapchain() noexcept;
        Error create_image_views() noexcept;
        Error create_depth_resources() noexcept;
        Error create_render_pass() noexcept;
        Error create_framebuffers() noexcept;
        Error create_sync_objects() noexcept;

        VkSurfaceFormatKHR choose_surface_format(const SwapChainSupportDetails::SurfaceFormat& formats) const noexcept;
        VkPresentModeKHR choose_present_mode(const SwapChainSupportDetails::PresentMode& modes) const noexcept;
        VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& caps) const noexcept;

        EngineDevice& device_;
        VkExtent2D window_extent_{};
        VkSwapchainKHR swapchain_{};

        VkFormat image_format_{};
        VkExtent2D extent_{};
        VkRenderPass render_pass_{};

        std::vector<VkImage> images_{};
        std::vector<VkImageView> image_views_{};
        std::vector<VkFramebuffer> framebuffers_{};

        std::vector<VkImage> depth_images_{};
        std::vector<VkDeviceMemory> depth_memorys_{};
        std::vector<VkImageView> depth_views_{};

        std::vector<VkSemaphore> sem_image_available_{};
        std::vector<VkSemaphore> sem_render_finished_{};
        std::vector<VkFence> fences_in_flight_{};
        std::vector<VkFence> fences_images_{};

        size_t frame_index_ = 0;
    };

}  // namespace hi