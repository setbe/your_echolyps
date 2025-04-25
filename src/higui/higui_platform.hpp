#pragma once

#if defined(_MSC_VER) && !defined(_DEBUG) && !defined(_CONSOLE)
// Forward declarations to avoid <Windows.h>
typedef struct HINSTANCE__ *HINSTANCE;
typedef char *LPSTR;

/* Detected Windows GUI application without console.
   The main() function was quietly changed to WinMain()
   main()'s arguments are not available. */
#define main()                                                                 \
    __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,            \
                      LPSTR lpCmdLine, int nCmdShow)
#endif

#include "higui_types.hpp"
#include <assert.h>

namespace hi {

void trim_working_set() noexcept;

// Allocate OS memory, use `hi::free(ptr, size)`
void *alloc(unsigned size) noexcept;

// Release OS memory back
void free(void *ptr, unsigned size) noexcept;

// Forced current process exit
int exit(int) noexcept;

void panic(Result) noexcept;

void panic_notify(Error, const char *msg) noexcept;
void sleep(unsigned ms) noexcept;

// returns seconds
double time() noexcept;

// ===== Contains all info related to crossplatform window management =====
namespace window {
Handler create(const Callback *, GraphicsContext &, int width,
               int height) noexcept;
void setup_opengl_context(const Handler HI_RESTRICT,
                          const GraphicsContext HI_RESTRICT) noexcept;
void swap_buffers(const GraphicsContext) noexcept;
void destroy(Handler handler) noexcept;

// `false` = window has been closed
bool poll_events(const Handler handler) noexcept;

// is window valid
bool is_valid(const Handler handler) noexcept;

void get_size(const Handler, int &width, int &height) noexcept;

#ifdef _WIN32 // args: const void* restict, const char* restict
void set_title(const Handler HI_RESTRICT handler,
               const char *HI_RESTRICT title) noexcept;
#else // args: int, const char*
void set_title(Handler handler, const char *title) noexcept;
#endif

void set_fullscreen(const Handler, bool enabled) noexcept;

void load_gl() noexcept;

#ifdef _WIN32
void create_class() noexcept;
#else
// No need for this. We need create `window class` on Windows only
#endif

} // namespace window
} // namespace hi