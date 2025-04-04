#pragma once

#ifndef RESTRICT
#ifdef __GNUC__
#define RESTRICT __restrict__
#elif defined(_MSC_VER)
#define RESTRICT __restrict
#else
#define RESTRICT
#endif
#endif // !RESTRICT

namespace hi {
    // ===== Internal stuff =====
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
} // namespace hi