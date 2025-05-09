#ifdef _WIN32
#include "types.hpp"

#define HIGUI_WINDOW_CLASSNAME L" "
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef CreateWindow

#include <Psapi.h> // psapi.lib

#include <assert.h>
#include <gl/GL.h>

#include "../../vs/resource.h" // take header from `vs` directory
#include "../external/wglext.h"

extern "C" const int _fltused = 0;

namespace hi {
// Key states (pressed = true, otherwise false)
unsigned key[256] = {0};
} // namespace hi

static LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                 LPARAM lparam) {
    const hi::Callback *callback = reinterpret_cast<hi::Callback *>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    hi::window::Handler handler = reinterpret_cast<hi::window::Handler>(hwnd);

    switch (msg) {
    case WM_MOUSEMOVE: {
        int mouse_x = LOWORD(lparam);
        int mouse_y = HIWORD(lparam);
        callback->mouse_move(*callback, mouse_x, mouse_y);
        return 0;
    }

    case WM_KEYDOWN:
        if (wparam < 256) {
            hi::key[wparam] = 1;
            return 0;
        }
        break;

    case WM_PAINT:
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;

    case WM_KEYUP:
        if (wparam < 256) {
            hi::key[wparam] = 0;
            callback->key_up(*callback, static_cast<int>(wparam));
        }
        return 0;

    case WM_SIZE: {
        int width = LOWORD(lparam);  // new width
        int height = HIWORD(lparam); // new height
        callback->resize(*callback, width, height);

        return 0;
    }

    case WM_SETFOCUS:
        callback->focus_gained(*callback);
        return 0;

    case WM_KILLFOCUS:
        callback->focus_lost(*callback);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_NCCREATE: {
        auto cs = reinterpret_cast<CREATESTRUCT *>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        break;
    }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

namespace hi {
void trim_working_set() noexcept {
#ifdef _PSAPI_H_ // Clear resources
    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
    EmptyWorkingSet(GetCurrentProcess());
#endif
} // trim_working_set

void *alloc(size_t size) noexcept {
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE);
} // alloc

void free(void *ptr, size_t /* size */) noexcept {
    VirtualFree(ptr, 0, MEM_RELEASE);
} // free

int exit(int code) noexcept {
#if defined(_WIN32) && defined(NDEBUG) && !defined(_CONSOLE)
    ExitProcess(code);
#endif
    // When it's debug mode we return int 0
    return 0;
} // exit

void panic_notify(Error error, const char *msg) noexcept {
    static_assert(sizeof(error) == 4U && sizeof(Error) == 4U);
    MessageBoxExA(nullptr, msg, "Error", MB_ICONERROR | MB_OK,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
    hi::exit(static_cast<int>(error));
}

void panic(Result result) noexcept {
    static_assert(sizeof(result.stage) == 4U && sizeof(Stage) == 4U);
    static_assert(sizeof(result.error) == 4U && sizeof(Error) == 4U);
    static_assert(sizeof(Result) == 8U);

    char message[] = "Crash: S00 C00\0";
    static_assert(sizeof(message) == 16); // 15 + '\0'
    constexpr unsigned index_stage = 7;
    constexpr unsigned index_code = 11;
    // values to display
    const unsigned char stage_val =
        static_cast<unsigned char>(static_cast<unsigned>(result.stage) % 100);
    const unsigned char code_val =
        static_cast<unsigned char>(static_cast<unsigned>(result.error) % 100);
    // insert values
    message[index_stage + 1] = '0' + (stage_val / 10);
    message[index_stage + 2] = '0' + (stage_val % 10);
    message[index_code + 1] = '0' + (code_val / 10);
    message[index_code + 2] = '0' + (code_val % 10);
    panic_notify(result.error, message);
}

void sleep(unsigned ms) noexcept { Sleep(ms); }
} // namespace hi

namespace hi::window {
void create_class() noexcept {
#if !defined(HI_MULTITHREADING_USED)
    static HMODULE instance = GetModuleHandleW(nullptr);
    static unsigned is_wc_registed = 0;
#else  // use multithreading
    thread_local HMODULE instance = GetModuleHandleW(nullptr);
    thread_local unsigned is_wc_registed = 0;
#endif // HI_MULTITHREADING_USED

    if (!is_wc_registed) { // Register window class
        WNDCLASS wc = {
            .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = win_proc,
            .hInstance = instance,
            .lpszClassName = HIGUI_WINDOW_CLASSNAME,
        };
        if (!RegisterClassW(&wc))
            panic({Stage::CreateWindow, Error::CreateWindowClassname});
        is_wc_registed = 1;
    }
}

Handler create(const Callback *callbacks, GraphicsContext &graphics_context,
               int width, int height) noexcept {
#if !defined(HI_MULTITHREADING_USED)
    static HMODULE instance = GetModuleHandleW(nullptr);
#else  // use multithreading
    thread_local HMODULE instance = GetModuleHandleW(nullptr);
#endif // HI_MULTITHREADING_USED

    create_class();

    HWND hwnd = CreateWindowExW(0, HIGUI_WINDOW_CLASSNAME, L"",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                                CW_USEDEFAULT, width, height, nullptr, nullptr,
                                instance, const_cast<Callback *>(callbacks));
    if (!hwnd)
        panic({Stage::CreateWindow, Error::CreateWindow});

    graphics_context = reinterpret_cast<GraphicsContext>(GetDC(hwnd));

    SendMessageW(hwnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadIcon(instance, MAKEINTRESOURCE(IDI_APP_ICON)));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadIcon(instance, MAKEINTRESOURCE(IDI_APP_ICON)));
    ShowWindow(hwnd, SW_SHOWNORMAL);
    return reinterpret_cast<Handler>(hwnd);
} // create

void swap_buffers(const GraphicsContext graphics_context) noexcept {
    SwapBuffers(reinterpret_cast<HDC>(graphics_context));
} // swap buffers

void destroy(Handler handler) noexcept {
    DestroyWindow(reinterpret_cast<HWND>(handler));
} // destroy

bool poll_events(const Handler handler) noexcept {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
} // poll_events

bool is_valid(const Handler handler) noexcept {
    return handler != nullptr;
} // is valid

void set_title(const Handler HI_RESTRICT handler,
               const char *HI_RESTRICT title) noexcept {
    wchar_t wide_buf[256];
    int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_buf, 256);

    // Mask: 0xFFFFFFFF if len > 0, otherwise 0x00000000
    uintptr_t mask = static_cast<uintptr_t>(-static_cast<intptr_t>(len > 0));

    // Compute pointer: if len > 0 � use wide_buf, otherwise use L""
    const wchar_t *result = (const wchar_t *)(((uintptr_t)L"" & ~mask) |
                                              ((uintptr_t)wide_buf & mask));

    SetWindowTextW(reinterpret_cast<HWND>(handler), result);
} // set title

void get_size(const Handler handler, int &width, int &height) noexcept {
    RECT rect{};
    if (!GetClientRect(reinterpret_cast<HWND>(handler), &rect)) {
        width = height = 0;
        return;
    }
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
} // get_size

void setup_opengl_context(const Handler HI_RESTRICT handler,
                          const GraphicsContext HI_RESTRICT
                              graphics_context) noexcept {
    // 1. Create a dummy window for loading WGL extensions
    WNDCLASSA dummy_class = {.style = CS_OWNDC,
                             .lpfnWndProc = DefWindowProcA,
                             .hInstance = GetModuleHandleA(nullptr),
                             .lpszClassName = "d"};

    if (!RegisterClassA(&dummy_class))
        panic({Stage::CreateWindow, Error::CreateDummyWindowClassname});

    HWND dummy_window = CreateWindowExA(
        0, dummy_class.lpszClassName, " ", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, 1, 1, nullptr, nullptr, dummy_class.hInstance, nullptr);

    if (!dummy_window)
        panic({Stage::CreateWindow, Error::CreateDummyWindow});

    // 1.2. Create dummy context for loading WGL extensions
    HDC dummy_device = GetDC(dummy_window);
    PIXELFORMATDESCRIPTOR pf_descriptor = {
        .nSize = sizeof(pf_descriptor),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .iLayerType = PFD_MAIN_PLANE};
    int pixel_format = ChoosePixelFormat(dummy_device, &pf_descriptor);
    if (pixel_format == 0 ||
        !SetPixelFormat(dummy_device, pixel_format, &pf_descriptor))
        panic({Stage::CreateWindow, Error::SetDummyPixelFormat});

    HGLRC dummy_context = wglCreateContext(dummy_device);
    wglMakeCurrent(dummy_device, dummy_context);

    // 2. Load WGL extensions
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)(void *)wglGetProcAddress(
            "wglChoosePixelFormatARB");
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)(void *)wglGetProcAddress(
            "wglCreateContextAttribsARB");

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB)
        panic({Stage::CreateWindow, Error::NotSupportedRequiredWglExtensions});

    // Cleanup dummy context
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_device);
    DestroyWindow(dummy_window);

    // 3. Set modern pixel format for the main window
    HDC device = reinterpret_cast<HDC>(graphics_context);
    const int pixel_attrs[] = {
        WGL_DRAW_TO_WINDOW_ARB,
        GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,
        GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,
        GL_TRUE,
        WGL_PIXEL_TYPE_ARB,
        WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,
        32,
        WGL_DEPTH_BITS_ARB,
        24,
        WGL_STENCIL_BITS_ARB,
        8,
        0 // The end of the array
    };

    int format;
    UINT num_format;
    wglChoosePixelFormatARB(device, pixel_attrs, nullptr, 1, &format,
                            &num_format);
    SetPixelFormat(device, format, &pf_descriptor);

    // 4. Create OpenGL 3.3 Core Profile context
    HGLRC gl_context;
    int context_attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                             3,
                             WGL_CONTEXT_MINOR_VERSION_ARB,
                             3,
                             WGL_CONTEXT_PROFILE_MASK_ARB,
                             WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                             0};

    gl_context = wglCreateContextAttribsARB(device, nullptr, context_attribs);

    // Create modern context
    if (!gl_context || !wglMakeCurrent(device, gl_context))
        panic({Stage::CreateWindow, Error::ModernOpenglContext});

    // Enable VSync
    typedef BOOL(APIENTRY * PFNWGLSWAPINTERVALEXTPROC)(int interval);
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)(void *)wglGetProcAddress(
            "wglSwapIntervalEXT");
    if (!wglSwapIntervalEXT)
        panic({Stage::CreateWindow, Error::EnableVSync});
    wglSwapIntervalEXT(1);
}

static void *opengl_loader(const char *name) noexcept {
    void *p = (void *)wglGetProcAddress(name);
    if (!p || p == (void *)0x1 || p == (void *)0x2 || p == (void *)0x3 ||
        p == (void *)-1) {
        static HMODULE module = LoadLibraryA("opengl32.dll");
        p = (void *)GetProcAddress(module, name);
    }
    return p;
}

void load_gl() noexcept {
    if (!wglGetCurrentContext() || !wglGetCurrentDC())
        panic({Stage::Opengl, Error::LoadOpenglFunctions});
    gladLoadGLLoader(opengl_loader);
}

void set_fullscreen(const Handler, bool enabled) noexcept {
    assert(false); // not implemented
}

} // namespace hi::window
#endif // _WIN32