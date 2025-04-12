#include "swap_chain.hpp"

namespace hi {
    SwapChain::~SwapChain() noexcept {
        // image view & framebuffer
        for (uint32_t i = 0; i < image_count_; i++) {
            vkDestroyImageView(device_.device(), image_views_[i], nullptr);
            vkDestroyFramebuffer(device_.device(), framebuffers_[i], nullptr);
        }

        // depth resources
        for (uint32_t i = 0; i < depth_images_count_; i++) {
            vkDestroyImageView(device_.device(), depth_views_[i], nullptr);
            vkDestroyImage(device_.device(), depth_images_[i], nullptr);
            vkFreeMemory(device_.device(), depth_memorys_[i], nullptr);
        }
        hi::free(depth_images_, sizeof(VkImage) * depth_images_count_);
        hi::free(depth_memorys_, sizeof(VkDeviceMemory) * depth_images_count_);
        hi::free(depth_views_, sizeof(VkImageView) * depth_images_count_);

        // sync objects
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device_.device(), sem_render_finished_[i], nullptr);
            vkDestroySemaphore(device_.device(), sem_image_available_[i], nullptr);
            vkDestroyFence(device_.device(), fences_in_flight_[i], nullptr);
        }
        hi::free(sem_render_finished_, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
        hi::free(sem_image_available_, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
        hi::free(fences_in_flight_, sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
        hi::free(fences_images_, sizeof(VkFence) * image_count_);

        if (render_pass_ != VK_NULL_HANDLE)
            vkDestroyRenderPass(device_.device(), render_pass_, nullptr);
        if (swapchain_ != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(device_.device(), swapchain_, nullptr);

        hi::free(image_views_, sizeof(VkImageView) * image_count_);
        hi::free(framebuffers_, sizeof(VkFramebuffer) * image_count_);
        hi::free(images_, sizeof(VkImage) * image_count_);
    }



    VkResult SwapChain::acquire_next_image(uint32_t* image_index) const noexcept {
        vkWaitForFences(device_.device(), 
            1, // fence count
            &fences_in_flight_[frame_index_], 
            VK_TRUE, 
            UINT64_MAX);
        return vkAcquireNextImageKHR(device_.device(), 
            swapchain_, 
            UINT64_MAX,
            sem_image_available_[frame_index_], 
            VK_NULL_HANDLE, 
            image_index);
    }

    VkResult SwapChain::submit_command_buffers(const VkCommandBuffer* buffers, uint32_t* image_index) noexcept {
        if (fences_images_[*image_index] != VK_NULL_HANDLE) {
            vkWaitForFences(device_.device(), 1, &fences_images_[*image_index], VK_TRUE, UINT64_MAX);
        }
        fences_images_[*image_index] = fences_in_flight_[frame_index_];

        VkSemaphore wait_semaphore = sem_image_available_[frame_index_];
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSemaphore signal_semaphore = sem_render_finished_[frame_index_];

        VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &wait_semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = buffers,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &signal_semaphore,
        };

        vkResetFences(device_.device(), 1, &fences_in_flight_[frame_index_]);
        if (vkQueueSubmit(device_.graphics_queue(), 1, &submit_info, fences_in_flight_[frame_index_]) != VK_SUCCESS)
            return VK_ERROR_DEVICE_LOST;

        VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &signal_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain_,
            .pImageIndices = image_index,
        };

        VkResult result = vkQueuePresentKHR(device_.present_queue(), &present_info);
        frame_index_ = (frame_index_ + 1) % MAX_FRAMES_IN_FLIGHT;
        return result;
    }

    Error SwapChain::create_swapchain() noexcept {
        SwapChainSupportDetails support = device_.get_swapchain_support();
        uint32_t desired_image_count = support.capabilities.minImageCount + 1;

        VkSurfaceFormatKHR surface_format = choose_surface_format(support.formats);
        VkPresentModeKHR present_mode = choose_present_mode(support.present_modes);
        VkExtent2D local_extent = choose_extent(support.capabilities);

        if (support.capabilities.maxImageCount > 0 &&
            desired_image_count > support.capabilities.maxImageCount) {
            desired_image_count = support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = device_.surface(),
            .minImageCount = desired_image_count,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = local_extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        };
        QueueFamilyIndices indices = device_.find_physical_queue_families();
        uint32_t queue_family_indices[] = { indices.graphics_family, indices.present_family };

        if (indices.graphics_family != indices.present_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices;
        }
        else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;      // Optional
            create_info.pQueueFamilyIndices = nullptr;  // Optional
        }

        create_info.preTransform = support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device_.device(), &create_info, nullptr, &swapchain_) != VK_SUCCESS) {
            return Error::CreateSwapChain;
        }

        vkGetSwapchainImagesKHR(device_.device(), swapchain_, &desired_image_count, nullptr);
        image_count_ = desired_image_count;
        images_ = static_cast<VkImage*>(hi::alloc(sizeof(VkImage) * image_count_));
        if (images_ == nullptr)
            return Error::InternalMemoryAlloc; // Couldn't allocate memory
        vkGetSwapchainImagesKHR(device_.device(), swapchain_, &desired_image_count, images_);

        image_format_ = surface_format.format;
        extent_ = local_extent;
        return Error::None;
    }

    Error SwapChain::create_image_views() noexcept {
        image_views_ = static_cast<VkImageView*>(hi::alloc(sizeof(VkImageView) * image_count_));
        if (image_views_ == nullptr)
            return Error::InternalMemoryAlloc; // Couldn't allocate memory

        for (uint32_t i = 0; i < image_count_; i++) {
            VkImageViewCreateInfo view_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = images_[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = image_format_,
                .subresourceRange {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            if (vkCreateImageView(device_.device(), &view_info, nullptr, &image_views_[i])
                != VK_SUCCESS) {
                return Error::CreateTextureImageView;
            }
        }
        return Error::None;
    }

    Error SwapChain::create_render_pass() noexcept { 
        Error error = Error::None;

        VkFormat depth_format = find_depth_format(error);
        if (error != Error::None) {
            return error; // Failed to find supported format
        }

        VkAttachmentDescription depth_attachment{
            .format = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        VkAttachmentReference depth_attachment_ref{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkAttachmentDescription color_attachment = {
            .format = image_format(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentReference color_attachment_ref = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_ref,
            .pDepthStencilAttachment = &depth_attachment_ref,
        };

        VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                           | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };
        constexpr size_t attachments_count{ 2 };
        VkAttachmentDescription attachments[attachments_count] {color_attachment, depth_attachment};
        VkRenderPassCreateInfo render_pass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachments_count),
            .pAttachments = attachments,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
        };

        if (vkCreateRenderPass(device_.device(), &render_pass_info, nullptr, &render_pass_)
            != VK_SUCCESS) {
            return Error::CreateRenderPass;
        }
        return Error::None; 
    }
    
    Error SwapChain::create_depth_resources() noexcept {
        Error error = Error::None;
        VkFormat depth_format = find_depth_format(error);
        if (error != Error::None) {
            return error; // Failed to find supported format
        }

        VkExtent2D local_extent = extent();
        depth_images_count_ = image_count_;
        depth_images_ = static_cast<VkImage*>(hi::alloc(sizeof(VkImage) * depth_images_count_));
        depth_memorys_ = static_cast<VkDeviceMemory*>(hi::alloc(sizeof(VkDeviceMemory) * depth_images_count_));
        depth_views_ = static_cast<VkImageView*>(hi::alloc(sizeof(VkImageView) * depth_images_count_));

        if (!depth_images_ || !depth_memorys_ || !depth_views_)
            return Error::InternalMemoryAlloc;

        for (uint32_t i = 0; i < depth_images_count_; ++i) {
            VkImageCreateInfo image_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                // .pNext = ,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = depth_format,
                .extent {
                    .width = local_extent.width,
                    .height = local_extent.height,
                    .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                // .queueFamilyIndexCount = uint32_t,
                // .pQueueFamilyIndices = const uint32_t*,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            device_.create_image_with_info(
                image_info,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depth_images_[i],
                depth_memorys_[i]);

            VkImageViewCreateInfo view_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = depth_images_[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = depth_format,
                .subresourceRange {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
            };

            if (vkCreateImageView(device_.device(), &view_info, nullptr, &depth_views_[i]) != VK_SUCCESS) {
                return Error::CreateTextureImageView;
            }
        }
        return Error::None; 
    }
    
    Error SwapChain::create_framebuffers() noexcept { 
        framebuffers_ = static_cast<VkFramebuffer*>(hi::alloc(sizeof(VkFramebuffer) * image_count_));
        for (uint32_t i = 0; i < image_count_; ++i) {
            VkImageView attachments[2] {image_views_[i], depth_views_[i]};

            VkExtent2D local_extent = extent();
            VkFramebufferCreateInfo framebuffer_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = render_pass_,
                .attachmentCount = 2,
                .pAttachments = attachments,
                .width = local_extent.width,
                .height = local_extent.height,
                .layers = 1,
            };
            if (vkCreateFramebuffer(device_.device(), &framebuffer_info, nullptr, &framebuffers_[i])
                != VK_SUCCESS) {
                return Error::CreateFramebuffer;
            }
        }
        return Error::None; 
    }
    
    Error SwapChain::create_sync_objects() noexcept { 
        sem_image_available_ = static_cast<VkSemaphore*>(hi::alloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT));
        sem_render_finished_ = static_cast<VkSemaphore*>(hi::alloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT));
        fences_in_flight_ = static_cast<VkFence*>(hi::alloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT));
        fences_images_ = static_cast<VkFence*>(hi::alloc(sizeof(VkFence) * image_count_));

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (vkCreateSemaphore(device_.device(), &semaphore_info, nullptr, &sem_image_available_[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device_.device(), &semaphore_info, nullptr, &sem_render_finished_[i]) != VK_SUCCESS ||
                vkCreateFence(device_.device(), &fence_info, nullptr, &fences_in_flight_[i]) != VK_SUCCESS) 
            {
                return Error::CreateSyncObjects;
            }
        }
        for (uint32_t i = 0; i < image_count_; i++)
            fences_images_[i] = VK_NULL_HANDLE;

        return Error::None; 
    }

    VkSurfaceFormatKHR SwapChain::choose_surface_format(const SwapChainSupportDetails::SurfaceFormat& formats) const noexcept {
        for (const auto& f : formats) {
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return f;
        }
        return formats[0];
    }

    VkPresentModeKHR SwapChain::choose_present_mode(const SwapChainSupportDetails::PresentMode& modes) const noexcept {
        for (const auto& m : modes) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
#ifndef NDEBUG
                debug::print_present_mode(m);
#endif // NDEBUG
                return m;
            }
        }
        // for (const auto &availablePresentMode : availablePresentModes) {
        //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        // #ifndef NDEBUG
        //     debug::print_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
        // #endif // NDEBUG
        //     return availablePresentMode;
        //   }
        // }
#ifndef NDEBUG
        debug::print_present_mode(VK_PRESENT_MODE_FIFO_KHR);
#endif // NDEBUG
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::choose_extent(const VkSurfaceCapabilitiesKHR& caps) const noexcept {
        if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
        VkExtent2D actual = window_extent_;

        VkExtent2D max = caps.maxImageExtent;
        VkExtent2D min = caps.minImageExtent;

        // set `min` if `less`
        // set `max` if `greater`
        
        actual.width =
            (actual.width < min.width) ? min.width :
            (actual.width > max.width) ? max.width :
            actual.width;

        actual.height =
            (actual.height < min.height) ? min.height :
            (actual.height > max.height) ? max.height :
            actual.height;

        return actual;
    }

    VkFormat SwapChain::find_depth_format(Error& error) const noexcept {
        constexpr size_t formats_count{ 3 };
        constexpr VkFormat formats[formats_count]{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
        return device_.find_supported_format(
            error,
            formats,
            formats_count,
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

}  // namespace hi
