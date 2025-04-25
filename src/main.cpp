#include "engine.hpp"
#include "external/linmath.h"
#include "higui/higui_debug.hpp"

void hi::Engine::start() noexcept {
    surface.set_title("Your Echolyps");

    text.init();
    text.add_text("Hello, OpenGL!", -0.4f, 0.0f, 0.005f);
    text.upload();
}

void hi::Engine::draw() const noexcept {
    opengl.clear();
    text.draw();
    surface.swap_buffers();
}

int main() {
    double g_start_time = hi::time();
    printf("time start: %f\n", g_start_time);

    hi::Engine engine{800, 600};
    g_start_time = hi::time();
    printf("time end: %f\n", g_start_time);
    float current_time = hi::time();
    // `false` means user closed the window
    while (engine.surface.poll_events()) {
        engine.draw();
    }
    g_start_time = hi::time();
    printf("time exit: %f\n", g_start_time);
    return 0;
}