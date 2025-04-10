#include "higui/higui.hpp"
#include "vulkan/device.hpp"

#ifdef _WIN32
extern void __stdcall ExitProcess(unsigned int);
#endif

using Handler = hi::window::Handler;

// Function prototypes (defined after the `main()`)
void update(Handler) noexcept;
void resize(Handler, int width, int height) noexcept;
void mouse_move(Handler, int x, int y) noexcept;
void key_up(Handler, int key) noexcept;
void focus_lost(Handler) noexcept;

int main() {
    hi::Surface surface(800, 600);
    if (surface.is_handler() == false) {
        hi::window::show_str_error("Error", "Failed to create the window");
        return static_cast<int>(hi::Error::WindowInit);
    }

    surface.set_title("Your Echolyps");

    hi::callback::update = update;
    hi::callback::resize = resize;
    hi::callback::mouse_move = mouse_move;
    hi::callback::key_up = key_up;
    hi::callback::focus_lost = focus_lost;

    EngineDevice device(surface);
    { // Init engine device
        hi::Result device_result = device.init();

        if (device_result.error_code != hi::Error::None) {
            hi::window::show_error(surface.get_handler(),
                device_result.stage_error, device_result.error_code);

            return static_cast<int>(device_result.error_code);
        }
    }
    hi::trim_working_set();
    surface.loop();

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