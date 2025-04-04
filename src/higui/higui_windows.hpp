#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(_DEBUG) && !defined(_CONSOLE)

    // Forward declarations to avoid <Windows.h>
    typedef struct HINSTANCE__* HINSTANCE;
    typedef char* LPSTR;

    /* Detected Windows GUI application without console.
       The main() function was quietly changed to WinMain()
       main()'s arguments are not available. */
#define main() \
    __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif

#ifdef __cplusplus
}
#endif

#include "higui.hpp"
#include "higui_utils.hpp"

namespace hi {
    void clean_memory() noexcept;

    namespace window {
        Handler create(int width, int height) noexcept;
        void destroy(Handler handler) noexcept;
        void loop(Handler handler) noexcept;

#ifdef _WIN32 
        // args: void*, char*
        void set_title(Handler RESTRICT handler, const char* RESTRICT title) noexcept;
#else
        // args: int, char*
        void set_title(Handler handler, const char* title) noexcept;
#endif
    } // namespace window
} // namespace hi