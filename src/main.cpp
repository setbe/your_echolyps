#include "higui/higui.hpp"
#include "vulkan/device.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/swap_chain.hpp"

using Handler = hi::window::Handler;
constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;

// Function prototypes (defined after the `main()`)
void update(Handler) noexcept;
void resize(Handler, int width, int height) noexcept;
void mouse_move(Handler, int x, int y) noexcept;
void key_up(Handler, int key) noexcept;
void focus_lost(Handler) noexcept;

int main() {
    hi::Surface surface;
    
    { // Init/Configure window
        hi::Result result = surface.init(WIDTH, HEIGHT);
        HI_CHECK_RESULT_AND_EXIT(result);

        surface.set_title("Your Echolyps");
        hi::callback::update = update;
        hi::callback::resize = resize;
        hi::callback::mouse_move = mouse_move;
        hi::callback::key_up = key_up;
        hi::callback::focus_lost = focus_lost;
    }

    hi::EngineDevice device{ surface };
    { // Init engine device
        hi::Result device_result = device.init();
        HI_CHECK_RESULT_AND_EXIT(device_result);
    }
    hi::SwapChain swap_chain{device, surface.get_extent()};
    { // Init swap chain
        hi::Result swap_chain_result = swap_chain.init();
        HI_CHECK_RESULT_AND_EXIT(swap_chain_result);
    }

    hi::Pipeline pipeline{ device };
    VkPipelineLayout pipeline_layout;
    { // Create pipeline
        hi::Result result{
            .stage_error = hi::StageError::CreatePipelineLayout,
            .error_code = hi::Error::None
        };
        result.error_code =
            pipeline.create_pipeline_layout(pipeline_layout);
        HI_CHECK_RESULT_AND_EXIT(result);
        
        hi::PipelineConfigInfo pipeline_config = hi::Pipeline::default_config_info(swap_chain.width(), swap_chain.height());
        pipeline_config.render_pass = swap_chain.render_pass();
        pipeline_config.pipeline_layout = pipeline_layout;

        hi::Result pipeline_result = pipeline.init(pipeline_config);
        HI_CHECK_RESULT_AND_EXIT(pipeline_result);
    }

    { // Run
        hi::trim_working_set();
        surface.loop();
    }

    { // Clean
        vkDestroyPipelineLayout(device.device(), pipeline_layout, nullptr);
    }

    return hi::exit(0);
}

void update(Handler) noexcept {

}

void resize(Handler, int width, int height) noexcept {

}

void mouse_move(Handler, int x, int y) noexcept {

}

void key_up(Handler, int key) noexcept {
}

void focus_lost(Handler) noexcept {
}