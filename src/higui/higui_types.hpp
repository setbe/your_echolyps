#pragma once

#ifdef _WIN32
extern "C" const int _fltused;
#endif // !_WIN32

#ifndef HI_RESTRICT
#ifdef __GNUC__
#define HI_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define HI_RESTRICT __restrict
#else
#define HI_RESTRICT
#endif
#endif // !HI_RESTRICT

#include "../external/glad.hpp"

namespace hi {
    // ===== Window stuff =====
    namespace window {
        // ===== Contains all info related to window backend =====
        enum class Backend {
            Unknown,
            X11,
            WindowsAPI,
            Cocoa,
            AndroidNDK,
        };

        consteval Backend get_window_backend() {
#if defined(__linux__)                 // Linux
            return Backend::X11
#elif defined(_WIN32)                  // Windows
            return Backend::WindowsAPI;
#elif defined(__APPLE__)               // Apple
            return Backend::Cocoa;
#elif defined(__ANDROID__)             // Android
            return Backend::AndroidNDK;
#else                                  // Unknown
            return Backend::Unknown;
#endif
        }

        template<Backend backend>
        struct Selector;

        template<> struct Selector<Backend::WindowsAPI> {
            using Handler = void*; // hWnd
            using DeviceContext = void*; // hDc
        };

        using Handler = Selector<get_window_backend()>::Handler;
        using GraphicsContext = Selector<get_window_backend()>::DeviceContext;
    } // namespace window

    // ===== Callback stuff =====
    struct Callback;

    namespace internal {
        inline void noop(const Callback&) noexcept {}
        inline void noop_int(const Callback&, int) noexcept {}
        inline void noop_int_int(const Callback&, int, int) noexcept {}

        using VoidCallback = void (*)(const Callback&);
        using VoidCallbackInt = void (*)(const Callback&, int);
        using VoidCallbackIntInt = void (*)(const Callback&, int, int);
    } // namespace internal

    struct Callback {
        inline Callback(void* user_data) noexcept
            : 
            user_data{ user_data },
            resize{ internal::noop_int_int },
            mouse_move{ internal::noop_int_int },
            key_up{ internal::noop_int },
            focus_gained{ internal::noop },
            focus_lost{ internal::noop }
        { }

        internal::VoidCallbackIntInt resize;
        internal::VoidCallbackIntInt mouse_move;
        internal::VoidCallbackInt key_up;
        internal::VoidCallback focus_gained;
        internal::VoidCallback focus_lost;

        template <typename T>
        inline T* get_user_data() const noexcept {
            return reinterpret_cast<T*>(user_data);
        }

    private:
        void* user_data;
    }; // struct Callback

    // Key states (pressed — true, otherwise — false)
    extern unsigned key[256];

    enum class Stage : unsigned {
        CreateWindow,
        Opengl,

        __Count__,
        __Max__ = 100
    }; // !Stage

    enum class Error : unsigned {
        None,
        InternalMemoryAlloc,

        CreateDummyWindowClassname,
        CreateDummyWindow,

        SetDummyPixelFormat,
        NotSupportedRequiredWglExtensions,
        ModernOpenglContext,
        EnableVSync,

        CreateWindowClassname,
        CreateWindow,
        LoadOpenglFunctions,

        CompileShader,
        CreateShaderProgram,

        __Count__,
        __Max__ = 100
    }; // !Error

    struct Result {
        Stage stage;
        Error error;
    };
} // namespace hi