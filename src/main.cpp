#include "engine.hpp"
#include "external/linmath.h"
#include "higui/higui_debug.hpp"

void hi::Engine::draw() const noexcept {
    glUseProgram(opengl.get_shader_program());
    glUniform3f(opengl.get_location_color(), 0.0f, opengl.get_green_value(),
                0.0f);
    opengl.draw();
    surface.swap_buffers();
}

int main() {
    double g_start_time = hi::time();
    printf("time start: %f\n", g_start_time);

    hi::Engine engine{800, 600, "Your Echolyps"};
    g_start_time = hi::time();
    printf("time end: %f\n", g_start_time);
    float current_time = hi::time();
    // `false` means user closed the window
    while (engine.surface.poll_events()) {
        engine.draw();
        engine.opengl.update_green_value();
    }
    g_start_time = hi::time();
    printf("time exit: %f\n", g_start_time);
    return 0;
}