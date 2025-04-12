#pragma once

#include "higui/higui.hpp"
#include "vulkan/device.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/swap_chain.hpp"

using Handler = hi::window::Handler;
constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;

namespace hi {

    struct Engine {
        Surface surface;
        EngineDevice device;
        SwapChain swap_chain;
        Pipeline pipeline;
        VkPipelineLayout pipeline_layout;

        VkCommandBuffer* command_buffers;
        uint32_t command_buffers_count;

        inline explicit Engine() noexcept
            : surface{},
            device{ surface },
            swap_chain{ device, surface.get_extent() },
            pipeline{ device },
            pipeline_layout{ nullptr },
            command_buffers{ nullptr }
        {}

        inline ~Engine() noexcept {
            vkDestroyPipelineLayout(device.device(), pipeline_layout, nullptr);
            hi::free(command_buffers, sizeof(VkCommandBuffer) * command_buffers_count);
        }

        // zero = success
        int init() noexcept {
            Result result = surface.init(WIDTH, HEIGHT);
            HI_CHECK_RESULT_AND_EXIT(result);

            surface.set_title("Your Echolyps");

            result = device.init();
            HI_CHECK_RESULT_AND_EXIT(result);

            result = swap_chain.init();
            HI_CHECK_RESULT_AND_EXIT(result);

            result.stage_error = StageError::CreatePipelineLayout;
            result.error_code = pipeline.create_pipeline_layout(pipeline_layout);
            HI_CHECK_RESULT_AND_EXIT(result);

            hi::PipelineConfigInfo pipeline_config = hi::Pipeline::default_config_info(swap_chain.width(), swap_chain.height());
            pipeline_config.render_pass = swap_chain.render_pass();
            pipeline_config.pipeline_layout = pipeline_layout;

            result = pipeline.init(pipeline_config);
            HI_CHECK_RESULT_AND_EXIT(result);

            result = create_command_buffers();
            HI_CHECK_RESULT_AND_EXIT(result);

            return 0;
        }

        inline void run() noexcept {
            trim_working_set();

            while (surface.poll_events())
            {
                draw_frame();
            }

            vkDeviceWaitIdle(device.device());
        }

        Error draw_frame() noexcept;

    private:
        Result create_command_buffers() noexcept;
    };

} // namespace hi