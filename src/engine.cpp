#include "engine.hpp"

namespace hi {

    Result Engine::create_command_buffers() noexcept {
        Result result{
            .stage_error = StageError::CreateCommandBuffers,
            .error_code = Error::None,
        };
        command_buffers_count = swap_chain.image_count();

        command_buffers = static_cast<VkCommandBuffer*>(hi::alloc(sizeof(VkCommandBuffer) * command_buffers_count));
        if (command_buffers == nullptr) {
            result.error_code = Error::InternalMemoryAlloc;
            return result;
        }

        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = device.command_pool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = command_buffers_count,
        };

        if (vkAllocateCommandBuffers(device.device(), &alloc_info, command_buffers) != VK_SUCCESS) {
            result.error_code = Error::AllocateCommandBuffers;
            return result;
        }

        for (uint32_t i = 0; i < command_buffers_count; ++i) {
            VkCommandBufferBeginInfo begin_info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            };
            if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS) {
                result.error_code = Error::BeginCommandBuffer;
                return result;
            }

            VkRenderPassBeginInfo render_pass_info{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = swap_chain.render_pass(),
                .framebuffer = swap_chain.framebuffer(i),
                .renderArea {
                    .offset = {0, 0},
                    .extent = swap_chain.extent(),
                },
            };

            VkClearValue clear_values[2];
            clear_values[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
            clear_values[1].depthStencil = { 1.0f, 0 };
            render_pass_info.clearValueCount = 2;
            render_pass_info.pClearValues = clear_values;

            vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            pipeline.bind(command_buffers[i]);
            vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

            vkCmdEndRenderPass(command_buffers[i]);
            if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
                result.error_code = Error::EndCommandBuffer;
                return result;
            }
        }

        return result;
    }

    Error Engine::draw_frame() noexcept {
        uint32_t image_index;
        auto result = swap_chain.acquire_next_image(&image_index);

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return Error::AcquireNextImage;
        }

        result = swap_chain.submit_command_buffers(&command_buffers[image_index], &image_index);

        if (result != VK_SUCCESS) {
            return Error::SubmitCommandBuffers;
        }

        return Error::None;
    }

} // namespace hi