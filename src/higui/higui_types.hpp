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

        constexpr Backend get_window_backend() {
#if defined(__linux__)
            return Backend::X11
#elif defined(_WIN32)
            return Backend::WindowsAPI;
#elif defined(__APPLE__)
            return Backend::Cocoa;
#elif defined(__ANDROID__)
            return Backend::AndroidNDK;
#else
            return Backend::Unknown;
#endif
        }

        template<Backend backend>
        struct Selector;

        template<> struct Selector<Backend::WindowsAPI> {
            using Handler = void*;
        };

        using Handler = Selector<get_window_backend()>::Handler;
    } // namespace internal

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
            : user_data { user_data },
            update{ internal::noop },
            resize{ internal::noop_int_int },
            mouse_move{ internal::noop_int_int },
            key_up{ internal::noop_int },
            key_down{ internal::noop_int },
            focus_gained{ internal::noop },
            focus_lost{ internal::noop }
        { }

        internal::VoidCallback update;
        internal::VoidCallbackIntInt resize;
        internal::VoidCallbackIntInt mouse_move;
        internal::VoidCallbackInt key_up;
        internal::VoidCallbackInt key_down;
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
    extern unsigned char key[256];

    enum class StageError : unsigned char {
        Opengl,
        CreateWindow,

        __Count__,
        __Max__ = 99
    }; // !StageError

    enum class Error : unsigned char {
        None,
        InternalMemoryAlloc,

        CreateDummyWindowClassname,
        CreateDummyWindow,

        SetDummyPixelFormat,
        NotSupportedRequiredWglExtensions,
        ModernOpenglContext,

        CreateWindowClassname,
        CreateWindow,
        LoadOpenglFunctions,

        __Count__,
        __Max__ = 99
    }; // !Error

    struct Result {
        StageError stage_error;
        Error error_code;
    };
} // namespace hi