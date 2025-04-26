#include "engine.hpp"
#include "external/linmath.h"
#include "higui/higui_debug.hpp"

void hi::Engine::start() noexcept {
    surface.set_title("Your Echolyps");

    text.init();
    text.add_text("hello, дивний світ! お風呂に入る ", -0.3f, 0.0f, 0.005f);
    text.upload();
}

void hi::Engine::draw() const noexcept {
    opengl.clear();
    text.draw();
    surface.swap_buffers();
}

int main() {
    hi::Engine engine{800, 600};
    // `false` means user closed the window
    while (engine.surface.poll_events()) {
        engine.draw();
    }
    return 0;
}