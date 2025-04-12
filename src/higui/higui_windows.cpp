#include "higui_types.hpp"
#include "higui_event.hpp"

#define HIGUI_WINDOW_CLASSNAME L" "
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef CreateWindow
#include <Psapi.h> // psapi.lib
#include <vulkan/vulkan_win32.h>

#include "../../vs/resource.h"

static LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    using namespace hi::callback;
    hi::window::Handler handler = reinterpret_cast<hi::window::Handler>(hwnd);

    switch (msg) {
    case WM_MOUSEMOVE: {
        int mouse_x = LOWORD(lparam);
        int mouse_y = HIWORD(lparam);
        mouse_move(handler, mouse_x, mouse_y);
        return 0;
    }

    case WM_KEYDOWN:
        if (wparam < 256) {
            hi::key[wparam] = 1;
            key_down(handler, static_cast<int>(wparam));
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
            key_up(handler, static_cast<int>(wparam));
        }
        return 0;

    case WM_SIZE: {
        int width = LOWORD(lparam); // new width
        int height = HIWORD(lparam); // new height
        resize(handler, width, height);
        return 0;
    }

    case WM_SETFOCUS:
        focus_gained(handler);
        return 0;

    case WM_KILLFOCUS:
        focus_lost(handler);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

namespace hi {
    void trim_working_set() noexcept {
#ifdef _PSAPI_H_ // Clear resources
        SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
        EmptyWorkingSet(GetCurrentProcess());
#endif
    }

    void* alloc(size_t size) noexcept {
        return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }

    void free(void* ptr, size_t /* size */) noexcept {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }

    int exit(int code) noexcept {
#if defined(_WIN32) && defined(NDEBUG) && !defined(_CONSOLE)
        ExitProcess(code);
#endif
        return 0;
    }
}

namespace hi::window {
    // check the result for nullptr
    Handler create(int width, int height, ::hi::Result& result) noexcept {
        static HMODULE instance = GetModuleHandleW(NULL);
        static unsigned char is_wc_registed = 0;

        if (!is_wc_registed) { // Register window class
            WNDCLASS wc = {
                .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
                .lpfnWndProc = win_proc,
                .hInstance = instance,
                .lpszClassName = HIGUI_WINDOW_CLASSNAME,
            };
            if (!RegisterClassW(&wc)) {
                result.error_code = ::hi::Error::CreateWindowClassname;
                return nullptr;
            }
            is_wc_registed = 1;
        }

        HWND hwnd = CreateWindowExW(0, HIGUI_WINDOW_CLASSNAME, L"",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            NULL, NULL, instance, NULL);

        /*HWND hwnd = CreateWindowExW(0, HIGUI_WINDOW_CLASSNAME, L"", // NO RESIZING
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr, nullptr, instance, nullptr);*/

        if (hwnd) {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(instance, MAKEINTRESOURCE(IDI_APP_ICON)));
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(instance, MAKEINTRESOURCE(IDI_APP_ICON)));
        }
        else {
            result.error_code = ::hi::Error::CreateWindow;
        }

        return reinterpret_cast<Handler>(hwnd);
    }

    void destroy(Handler handler) noexcept {
        DestroyWindow(reinterpret_cast<HWND>(handler));
        handler = nullptr;
    }

    bool poll_events(const Handler handler) noexcept {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return false;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return true;
    }

    bool is_valid(const Handler handler) noexcept {
        return handler != nullptr;
    }

    void set_title(const Handler HI_RESTRICT handler, const char* HI_RESTRICT title) noexcept {
        wchar_t wide_buf[256];
        int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_buf, 256);

        // Mask: 0xFFFFFFFF if len > 0, otherwise 0x00000000
        uintptr_t mask = static_cast<uintptr_t>(-static_cast<intptr_t>(len > 0));

        // Compute pointer: if len > 0 — use wide_buf, otherwise use L""
        const wchar_t* result = (const wchar_t*)(((uintptr_t)L"" & ~mask) | ((uintptr_t)wide_buf & mask));

        SetWindowTextW(reinterpret_cast<HWND>(handler), result);
    }
    VkResult create_vulkan_surface(const Handler handler, VkInstance instance, VkSurfaceKHR* surface) noexcept {
        VkWin32SurfaceCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = reinterpret_cast<HWND>(handler),
        };
        return vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, surface);
    }

    void get_size(const Handler handler, int& width, int& height) noexcept {
        RECT rect{};
        if (GetClientRect(reinterpret_cast<HWND>(handler), &rect)) {
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }
        else {
            width = height = 0;
        }
    }

    void show_error(const Handler handler, StageError stage, Error code) noexcept {
        constexpr size_t message_length = 16;
        char message[message_length] = "Crash: S00 C00\0"; // S = stage, C = code
        constexpr size_t index_stage = 7;
        constexpr size_t index_code = 11;

        static_assert(sizeof(stage) == 1U && sizeof(Error) == 1U);

        const uint8_t stage_val = static_cast<uint8_t>(stage);
        const uint8_t code_val = static_cast<uint8_t>(code);

        // `stage` write
        message[index_stage + 1] = '0' + (stage_val / 10);
        message[index_stage + 2] = '0' + (stage_val % 10);

        // `error code` write
        message[index_code + 1] = '0' + (code_val / 10);
        message[index_code + 2] = '0' + (code_val % 10);

        MessageBoxExA(
            reinterpret_cast<HWND>(handler),
            message,
            "Error",
            MB_ICONERROR | MB_OK,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
        );
    }

    void show_str_error(const char* title, const char* str) noexcept {
        MessageBoxExA(
            nullptr,
            str,
            title,
            MB_ICONERROR | MB_OK,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
        );
    }


} // namespace hi::window