#include "higui.hpp"
#include "higui_windows.hpp"
#include "higui_utils.hpp"

#define HIGUI_WINDOW_CLASSNAME L" "
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h> // psapi.lib

namespace hi {
    void clean_memory() noexcept {
        // Clear resources
#ifdef _PSAPI_H_
        SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
        EmptyWorkingSet(GetCurrentProcess());
#endif
    }

    namespace window {
        Handler create(int width, int height) noexcept {
            static HMODULE instance = GetModuleHandleW(NULL);
            static unsigned char is_wc_registed = 0;

            if (!is_wc_registed) { // Register window class
                WNDCLASS wc = { 0 };
                
                wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
                wc.lpfnWndProc = DefWindowProcW;
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

        void destroy(Handler handler) noexcept {
            DestroyWindow(reinterpret_cast<HWND>(handler));
        }

        void loop(Handler handler) noexcept {
            MSG msg{};
            while (GetMessageW(&msg, reinterpret_cast<HWND>(handler), 0, 0) > 0) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        void set_title(Handler RESTRICT handler, const char* RESTRICT title) noexcept {
            wchar_t wide_buf[256];
            int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_buf, 256);

            // Mask: 0xFFFFFFFF if len > 0, otherwise 0x00000000
            uintptr_t mask = static_cast<uintptr_t>(-static_cast<intptr_t>(len > 0));

            // Compute pointer: if len > 0 — use wide_buf, otherwise use L""
            const wchar_t* result = (const wchar_t*)(((uintptr_t)L"" & ~mask) | ((uintptr_t)wide_buf & mask));

            SetWindowTextW(reinterpret_cast<HWND>(handler), result);
        }

    } // namespace window
} // namespace hi