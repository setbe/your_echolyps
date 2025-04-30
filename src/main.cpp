#include "engine.hpp"
#include "higui/higui_debug.hpp"

inline static bool show_debug_menu = false;
inline static unsigned skipped_chunks = 0;

constexpr int FPS_HISTORY_SIZE = 100;
static double dt_history[FPS_HISTORY_SIZE] = {};
static int dt_index = 0;
static int dt_count = 0;

void hi::Engine::start() noexcept {
    surface.set_title("Your Echolyps");
    text.init(font.font_bitmap);
}

void hi::Engine::update() noexcept {
    double dt = hi::delta_time();
    static float simple_timer{0};
    simple_timer += show_debug_menu ? dt : 0.f;

    dt_history[dt_index] = dt;
    dt_index = (dt_index + 1) % FPS_HISTORY_SIZE;
    if (dt_count < FPS_HISTORY_SIZE)
        dt_count++;

    double avg_dt = 0.0;
    for (int i = 0; i < dt_count; ++i)
        avg_dt += dt_history[i];
    avg_dt /= dt_count;
    unsigned avg_fps = avg_dt > 0.0 ? static_cast<unsigned>(1.0 / avg_dt) : 0;

    if (simple_timer > 0.1f) {
        unsigned fps = dt > 0.0 ? static_cast<unsigned>(1.0 / dt) : 0;
        text.add_text(-0.93f, 0.9f, 0.003f,
                      "x %f, y %f, z %f\n"
                      "fps: %d\n"
                      "fps avg: %d\n"
                      "delta: %f\n"
                      "skipped chunks: %d\n",
                      world.camera.position[0], // x
                      world.camera.position[1], // y
                      world.camera.position[2], // z
                      fps, avg_fps, static_cast<float>(dt), skipped_chunks);
        text.upload();
        simple_timer = 0.f;
    }

    bool should_update_view = false;

    if (key::Control_L)
        world.camera.movement_speed = 50.f;
    else
        world.camera.movement_speed = 5.0f;

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

    if (should_update_view)
        world.camera.look_at(world.view);
}

void hi::Engine::draw() const noexcept {
    opengl.clear();
    skipped_chunks = world.draw();
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
        e.draw();
    }
    return 0;
}