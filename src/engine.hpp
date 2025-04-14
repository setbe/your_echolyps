#pragma once

#include "higui/higui.hpp"
#include "vulkan/device.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/swap_chain.hpp"
#include "vulkan/model.hpp"

using Handler = hi::window::Handler;
constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;

namespace hi {

    struct Engine {
        Surface surface;
        EngineDevice device;
        SwapChain swap_chain;
        Pipeline pipeline;
        Model model;

        VkPipelineLayout pipeline_layout;
        VkCommandBuffer* command_buffers;
        uint32_t command_buffers_count;

        inline explicit Engine() noexcept
            : surface{},
            device{ surface },
            swap_chain{ device, surface.get_extent() },
            pipeline{ device },
            pipeline_layout{ nullptr },
            command_buffers{ nullptr },
            model{ device }
        {}
 
        inline ~Engine() noexcept {
            vkDestroyPipelineLayout(device.device(), pipeline_layout, nullptr);
            hi::free(command_buffers, sizeof(VkCommandBuffer) * command_buffers_count);
        }

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        // zero == success
        int init() noexcept {
            constexpr size_t BINDING_COUNT = 1;
            constexpr size_t ATTRIBUTE_COUNT = 1;
            
            Result result;
            
            // window
            result = surface.init(WIDTH, HEIGHT);
            HI_CHECK_RESULT_AND_EXIT(result);

            surface.set_title("Your Echolyps");

            // device
            result = device.init();
            HI_CHECK_RESULT_AND_EXIT(result);

            // swap chain
            result = swap_chain.init();
            HI_CHECK_RESULT_AND_EXIT(result);

            // model
            Model::Vertex mesh[3]{
                {{0.0f, -0.5f}, {1.f, 0.f, 0.f}},
                {{0.5f, 0.5f}, {0.f, 1.f, 0.f}},
                {{-0.5f, 0.5f}, {0.f, 0.f, 1.f}}
            };
            result = model.init(mesh, sizeof(mesh) / sizeof(mesh[0]));
            HI_CHECK_RESULT_AND_EXIT(result);

            // pipeline layout
            result.stage_error = StageError::CreatePipelineLayout;
            result.error_code = pipeline.create_pipeline_layout(pipeline_layout);
            HI_CHECK_RESULT_AND_EXIT(result);

            // pipeline info
            PipelineConfigInfo pipeline_config = hi::Pipeline::default_config_info(swap_chain.width(), swap_chain.height());
            pipeline_config.render_pass = swap_chain.render_pass();
            pipeline_config.pipeline_layout = pipeline_layout;

            // pipeline
            result = pipeline.init(pipeline_config, 1, 2);
            HI_CHECK_RESULT_AND_EXIT(result); 

            // command buffers
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