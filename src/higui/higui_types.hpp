#pragma once
extern "C" const int _fltused;

#ifndef HI_RESTRICT
#ifdef __GNUC__
#define RESTRICT __restrict__
#elif defined(_MSC_VER)
#define HI_RESTRICT __restrict
#else
#define HI_RESTRICT
#endif
#endif // !HI_RESTRICT

#include <vulkan/vulkan.h>

namespace hi {
    enum class StageError : uint8_t {
        CreateInstance,
        SetupDebugMessenger,
        CreateSurface,
        PickPhysicalDevice,
        CreateLogicalDevice,
        CreateCommandPool,
        CreateBuffer,
        CreateImageWithInfo,

        __Count__,
        __Max__ = 99
    };

    enum class Error : uint8_t {
        None,
        InternalMemoryAlloc,
        WindowInit,
        WindowClassnameInit,
        ValidationLayers,
        VulkanInstance,
        DebugMessenger,
        VulkanSupport,
        SuitableGpu,
        SurfaceKhr,
        LogicalDevice,
        PhysicalDevice,
        CommandPool,

        SuitableMemoryType,
        BufferInit,
        BufferAlloc,
        ImageInit,
        ImageAlloc,
        ImageBind,

        __Count__,
        __Max__ = 99
    };

    struct Result {
        StageError stage_error;
        Error error_code;
    };

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
    namespace callback {
        namespace internal {
            inline void void_noop_win(::hi::window::Handler) noexcept {}
            inline void void_noop_win_int(::hi::window::Handler, int) noexcept {}
            inline void void_noop_win_int_int(::hi::window::Handler, int, int) noexcept {}

            typedef void (*VoidCallbackHandler)(::hi::window::Handler);
            typedef void (*VoidCallbackHandlerInt)(::hi::window::Handler, int);
            typedef void (*VoidCallbackHandlerIntInt)(::hi::window::Handler, int, int);
        } // namespace internal
    } // namespace callback
} // namespace hi