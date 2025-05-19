#define HIGUI_GLYPH_USE_STATIC_DRAW
#include "engine/engine.hpp"

inline static bool show_debug_menu = false;

void hi::Engine::start() noexcept {
    surface.set_title("Your Echolyps");
    text.init(font.font_bitmap);
}

void hi::Engine::update() noexcept {
    double dt = hi::calculate_delta_time();
    static float simple_timer{0.f};

    // Render debug menu in the game
    if (show_debug_menu) {
        simple_timer += dt;
        if (simple_timer > 0.1f) {
            unsigned fps = dt > 0.0 ? static_cast<unsigned>(1.0 / dt) : 0;
            text.add_text(-0.93f, 0.9f, 0.003f,
                          "x %f y %f z %f\n"
                          "fps: %d\n"
                          "delta: %f\n",
                          world.camera.position[0], // x
                          world.camera.position[1], // y
                          world.camera.position[2], // z
                          fps, static_cast<float>(dt) * 1000);
            text.upload();
            simple_timer = 0.f;
        }
    }

    bool should_update_view = false;

    // Movement
    if (key::Control_L)
        world.camera.movement_speed = 100.f;
    else
        world.camera.movement_speed = 10.0f;

    if (key::w || key::W) {
        world.camera.move_forward(dt);
        should_update_view = true;
    }
    if (key::a || key::A) {
        world.camera.move_left(dt);
        should_update_view = true;
    }
    if (key::s || key::S) {
        world.camera.move_backward(dt);
        should_update_view = true;
    }
    if (key::d || key::D) {
        world.camera.move_right(dt);
        should_update_view = true;
    }
    if (key::space) {
        world.camera.move_up(dt);
        should_update_view = true;
    }
    if (key::Shift_L) {
        world.camera.move_down(dt);
        should_update_view = true;
    }

    if (should_update_view) {
        world.update_pos();
        world.camera.look_at(world.view);
    }
}

void hi::Engine::draw() const noexcept {
    opengl.clear();
    world.draw();
    if (show_debug_menu)
        text.draw();
    surface.swap_buffers();
}

void hi::Engine::key_up(const hi::Callback &cb, key::KeyCode key) noexcept {
    Engine *e = cb.get_user_data<Engine>();
    using key::KeyCode;
    switch (key) {
    case KeyCode::F3: {
        if (e->config.is_wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        e->config.is_wireframe = !e->config.is_wireframe;
    } break;

    case KeyCode::F2: {
        show_debug_menu = !show_debug_menu;
    } break;

    case KeyCode::F1: {
        e->surface.set_cursor_visible(e->config.is_cursor);
        e->config.is_cursor = !e->config.is_cursor;
    } break;

    case KeyCode::Escape: {
        e->surface.quit();
    } break;

    default:
        break;
    }
}

int main() {
    hi::Engine e{800, 600};

    // `false` means user closed the window
    while (e.surface.poll_events()) {
        e.update();
        e.world.update();
        e.draw();
    }
    return 0;
}