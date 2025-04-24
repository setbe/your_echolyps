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
#if defined(__linux__) // Linux
    return Backend::X11;
#elif defined(_WIN32)      // Windows
    return Backend::WindowsAPI;
#elif defined(__APPLE__)   // Apple
    return Backend::Cocoa;
#elif defined(__ANDROID__) // Android
    return Backend::AndroidNDK;
#else                      // Unknown
    return Backend::Unknown;
#endif
}

template <Backend backend> struct Selector;

template <> struct Selector<Backend::X11> {
    using Handler = unsigned int; // Window
    using DeviceContext = void *; // Display*
};

template <> struct Selector<Backend::WindowsAPI> {
    using Handler = void *;       // hWnd
    using DeviceContext = void *; // hDc
};

using Handler = Selector<get_window_backend()>::Handler;
using GraphicsContext = Selector<get_window_backend()>::DeviceContext;
} // namespace window

// ===== Callback stuff =====
struct Callback;

namespace internal {} // namespace internal

struct Callback {
    using Void = void (*)(const Callback &);
    using VoidInt = void (*)(const Callback &, int);
    using VoidIntInt = void (*)(const Callback &, int, int);

    inline Callback(void *user_data, VoidIntInt resize, VoidIntInt mouse_move,
                    VoidInt key_up, Void focus_gained, Void focus_lost) noexcept
        : user_data{user_data}, resize{resize}, mouse_move{mouse_move},
          key_up{key_up}, focus_gained{focus_gained}, focus_lost{focus_lost} {}

    VoidIntInt resize;
    VoidIntInt mouse_move;
    VoidInt key_up;
    Void focus_gained;
    Void focus_lost;

    template <typename T> inline const T *get_user_data() const noexcept {
        return reinterpret_cast<T *>(user_data);
    }

  private:
    void *user_data;
}; // struct Callback

// Key states (pressed � true, otherwise � false)
extern unsigned key[256];

enum class Stage : unsigned {
    CreateWindow,
    Opengl,
    LinuxSyscall,

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

    OpenDisplay,
    CreateWindowClassname,
    CreateWindow,
    LoadOpenglFunctions,

    CompileShader,
    CreateShaderProgram,

    SyscallGetTime,

    __Count__,
    __Max__ = 100
}; // !Error

struct Result {
    Stage stage;
    Error error;
};
} // namespace hi