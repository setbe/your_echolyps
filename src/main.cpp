#include "higui/higui.hpp"

#include <vulkan/vulkan.h>


// #include <stdio.h>
void on_resize(hi::window::Handler, int width, int height) noexcept {

}

void on_mouse_move(hi::window::Handler, int x, int y) noexcept {
    
}

void on_key_up(hi::window::Handler, int key) noexcept {
}

inline static int run() noexcept {
    hi::Surface surface(800, 600);
    if (surface.is_handler() == false)
        return -1;

    surface.set_title("Your Echolyps");

#ifdef _WIN32
    hi::clean_memory();
#endif // _WIN32

    hi::callback::resize = on_resize;
    hi::callback::mouse_move = on_mouse_move;
    hi::callback::key_up = on_key_up;
    surface.loop();

    return 0;
}

int main() {
    return run();
}