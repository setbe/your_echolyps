#include "engine.hpp"
#include "external/linmath.h"

void hi::Engine::start() noexcept {
    surface.set_title("Your Echolyps");

    text.init(font.font_bitmap);
    text.add_text("твій, echolyps!", -0.4f, 0.8f, 0.005f);
    text.upload();
}

void hi::Engine::draw() const noexcept {
    opengl.clear();
    text.draw();
    surface.swap_buffers();
}

void hi::Engine::key_up(const hi::Callback &cb, key::KeyCode key) noexcept {
    switch (key) {
    case key::KeyCode::F11: {
        static bool is_wireframe = false;
        if (is_wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        is_wireframe = !is_wireframe;
    } break;

    default:
        break;
    }
}

int main() {
    hi::Engine engine{800, 600};
    // `false` means user closed the window
    while (engine.surface.poll_events()) {
        engine.draw();
    }
    return 0;
}