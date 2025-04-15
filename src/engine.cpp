#include "engine.hpp"

// This macro shows `StageError` and `Error` numbers to user and exits if error occurs.
// Must be used in function, which returns `int`, in order to work properly.
#define HI_CHECK_STAGE_CODE_AND_EXIT() \
    if (error != hi::Error::None) { \
        hi::window::show_error({stage, error}); \
        return hi::exit(static_cast<int>(error)); \
    }

namespace hi {
    int Engine::init() noexcept {
        StageError stage = StageError::CreateWindow;
        Error error = Error::None;

        // window
        error = surface.init(WIDTH, HEIGHT, &callback);
        HI_CHECK_STAGE_CODE_AND_EXIT();

        stage = StageError::Opengl;
        if (window::load_gl() == 0)
            error = Error::LoadOpenglFunctions;
        HI_CHECK_STAGE_CODE_AND_EXIT();



        surface.set_title("Your Echolyps");
        callback.resize = framebuffer_resize_adapter;

        return 0;
    }
} // namespace hi