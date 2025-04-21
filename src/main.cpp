#include "engine.hpp"

int main() {
#if defined(_WIN32) && !defined(HI_MULTITHREADING_USED)
    hi::window::create_class();
#endif // _WIN32 && HI_MULTITHREADING_USED

    hi::Engine engine{ 800, 600, "Your Echolyps" };
    hi::trim_working_set();
    engine.run();

    return hi::exit(0);
}