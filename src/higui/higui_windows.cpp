#include "higui.hpp"
#include "higui_windows.hpp"
#include "higui_utils.hpp"
#include "higui_event.hpp"

#define HIGUI_WINDOW_CLASSNAME L" "
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h> // psapi.lib


LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    hi::window::Handler handler = reinterpret_cast<hi::window::Handler>(hwnd);
    switch (msg) {
    case WM_MOUSEMOVE: {
        int mouse_x = LOWORD(lparam);
        int mouse_y = HIWORD(lparam);
        hi::callback::mouse_move(handler, mouse_x, mouse_y);
        break;
    }

    case WM_KEYDOWN:
        if (wparam < 256) {
            hi::key[wparam] = 1;
            hi::callback::key_down(handler, static_cast<int>(wparam));
        }
        break;

    case WM_KEYUP:
        if (wparam < 256) {
            hi::key[wparam] = 0;
            hi::callback::key_up(handler, static_cast<int>(wparam));
        }
        break;

    case WM_PAINT:
        hi::callback::render(handler);
        break;

    case WM_SIZE: {
        int width = LOWORD(lparam); // new width
        int height = HIWORD(lparam); // new height
        hi::callback::resize(handler, width, height);
        break;
    }

    case WM_SETFOCUS:
        hi::callback::focus_gained(handler);
        break;
    

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

namespace hi {
    void clean_memory() noexcept {
        // Clear resources
#ifdef _PSAPI_H_
        SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
        EmptyWorkingSet(GetCurrentProcess());
#endif
    }

    // check the result for nullptr
    window::Handler window::create(int width, int height) noexcept {
        static HMODULE instance = GetModuleHandleW(NULL);
        static unsigned char is_wc_registed = 0;

        if (!is_wc_registed) { // Register window class
            WNDCLASS wc = { 0 };

            wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = win_proc;
            wc.hInstance = instance;
            wc.lpszClassName = HIGUI_WINDOW_CLASSNAME;

            if (!RegisterClassW(&wc))
                return nullptr;

            is_wc_registed = 1;
        }

        HWND hwnd = CreateWindowExW(0, HIGUI_WINDOW_CLASSNAME, L"",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            NULL, NULL, instance, NULL);

        return reinterpret_cast<Handler>(hwnd);
    }

    void window::destroy(window::Handler handler) noexcept {
        DestroyWindow(reinterpret_cast<HWND>(handler));
    }

    void window::loop(window::Handler handler) noexcept {
        MSG msg{};
        while (GetMessageW(&msg, reinterpret_cast<HWND>(handler), 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void window::set_title(window::Handler RESTRICT handler, const char* RESTRICT title) noexcept {
        wchar_t wide_buf[256];
        int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_buf, 256);

        // Mask: 0xFFFFFFFF if len > 0, otherwise 0x00000000
        uintptr_t mask = static_cast<uintptr_t>(-static_cast<intptr_t>(len > 0));

        // Compute pointer: if len > 0 — use wide_buf, otherwise use L""
        const wchar_t* result = (const wchar_t*)(((uintptr_t)L"" & ~mask) | ((uintptr_t)wide_buf & mask));

        SetWindowTextW(reinterpret_cast<HWND>(handler), result);
    }
} // namespace hi