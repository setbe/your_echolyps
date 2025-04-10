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

    void* alloc(size_t size) noexcept;
    void free(void* ptr, size_t size) noexcept;
    int exit(int) noexcept;

    namespace window {
        Handler create(int width, int height) noexcept;
        void destroy(Handler handler) noexcept;
        void loop(const Handler handler) noexcept;
        bool is_valid(const Handler handler) noexcept;

        VkResult create_vulkan_surface(
            const Handler handler,
            VkInstance instance,
            VkSurfaceKHR* surface) noexcept;

#ifdef _WIN32 
        // args: const void* restict, const char* restict
        void set_title(const Handler HI_RESTRICT handler, const char* HI_RESTRICT title) noexcept;
#else
        // args: int, const char*
        void set_title(Handler handler, const char* title) noexcept;
#endif

        void show_error(const Handler, StageError, Error) noexcept;
        void show_str_error(const char* title, const char* str) noexcept;
    } // namespace window
} // namespace hi