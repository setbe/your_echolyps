#include "higui/higui.hpp"
#include "vulkan/device.hpp"
#include "vulkan/pipeline.hpp"

#ifdef _WIN32
extern void __stdcall ExitProcess(unsigned int);
#endif

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
    hi::Surface surface{WIDTH, HEIGHT};

    { // Check window
        if (surface.is_handler() == false) {
            constexpr hi::Error error = hi::Error::WindowInit;
            hi::window::show_error(nullptr, hi::StageError::CreateWindow, error);
            return hi::exit(static_cast<int>(error));
        }
    }

    { // Setup window
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

        if (device_result.error_code != hi::Error::None) {
            hi::window::show_error(surface.get_handler(),
                device_result.stage_error, device_result.error_code);

            return hi::exit(static_cast<int>(device_result.error_code));
        }
    }

    hi::Pipeline pipeline{ device };
    { // Init graphics pipeline
        hi::PipelineConfigInfo default_pipeline_config_info = 
            hi::Pipeline::default_config_info(WIDTH, HEIGHT);
        
        hi::Error error = pipeline.init(default_pipeline_config_info);

        if (error != hi::Error::None) {
            hi::window::show_error(surface.get_handler(), hi::StageError::CreatePipeline, error);
            return hi::exit(static_cast<int>(error));
        }
    }

    { // Run
        hi::trim_working_set();
        surface.loop();
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