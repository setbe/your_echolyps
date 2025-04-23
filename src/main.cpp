#include "engine.hpp"

void hi::Engine::draw() const noexcept {
    this->opengl.draw();
    this->surface.swap_buffers();
}

int main() {
    // Create window, opengl context
    hi::Engine engine{800, 600, "Your Echolyps"};
    hi::trim_working_set(); // optional

    // `false` means user closed the window
    while (engine.surface.poll_events()) {
        engine.draw();
    }
    return 0;
}