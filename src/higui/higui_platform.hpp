#pragma once

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

#include "higui_types.hpp"

namespace hi {
    void trim_working_set() noexcept;

    void* alloc(size_t size) /* - Allocate OS memory, use free(ptr, size) - */ noexcept;
    void free(void* ptr, size_t size) /* - Release memory back - */ noexcept;
    int exit(int) /* - Forced current process exit - */ noexcept;

    namespace window {
        Handler create(int width, int height, ::hi::Error&, const Callback*) noexcept;
        Error setup_opengl_context(const Handler) noexcept;
        void swap_buffers(const Handler) noexcept;
        void destroy(Handler handler) noexcept;
        bool poll_events(const Handler handler) /* - false = window close - */ noexcept;
        bool is_valid(const Handler handler) /* - has valid value --------- */ noexcept;

        void get_size(const Handler, int& width, int& height) noexcept;

#ifdef _WIN32 // args: const void* restict, const char* restict
        void set_title(const Handler HI_RESTRICT handler, const char* HI_RESTRICT title) noexcept;
#else // args: int, const char*
        void set_title(Handler handler, const char* title) noexcept;
#endif
        void set_fullscreen(const Handler, bool enabled) noexcept;

        int load_gl() noexcept; // returns 0 as failure

        // --- error handling
        void show_error(Result) noexcept;
    } // namespace window
} // namespace hi