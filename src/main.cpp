#include "engine.hpp"

int main() {
    hi::Engine engine;
    {
        int r = engine.init();
        if (r != 0) return r;
    }
    engine.run();

    return hi::exit(0);
}