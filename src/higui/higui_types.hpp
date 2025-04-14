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

// This macro sets the structure { (uint8_t)stage, (uint8_t)code }
// and returns from the function if an error occurs
#define HI_STAGE_CHECK(stage_enum, func)         \
    stage = StageError::stage_enum;              \
    if ((code = func()) != Error::None)          \
        return Result { stage, code };

// This macro shows `hi::Result` numbers to user and exits if error occurs.
// Must be used in function, which returns `int`, in order to work properly.
#define HI_CHECK_RESULT_AND_EXIT(result) \
    if ((result).error_code != hi::Error::None) { \
        hi::window::show_error(nullptr, (result).stage_error, (result).error_code); \
        return hi::exit(static_cast<int>((result).error_code)); \
    }

#include <vulkan/vulkan.h>

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

    enum class StageError : uint8_t {
        CreateWindow,
        CreateInstance,
        SetupDebugMessenger,
        CreateSurface,
        PickPhysicalDevice,
        CreateLogicalDevice,
        CreateCommandPool,
        CreateBuffer,
        CreateImageWithInfo,

        CreateShaderModule,
        CreatePipeline,
        CreateSwapChain,
        CreateImageViews,
        CreateRenderPass,
        CreateDepthResources,
        CreateFramebuffers,
        CreateSyncObjects,
        CreatePipelineLayout,

        CreateCommandBuffers,
        ModelInit,

        __Count__,
        __Max__ = 99
    }; // !StageError

    enum class Error : uint8_t {
        None,
        InternalMemoryAlloc,
        CreateWindow,
        CreateWindowClassname,
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

        CreateShaderModule,
        GraphicsPipeline,

        CreateSwapChain,
        CreateTextureImageView,
        CreateRenderPass,
        CreateFramebuffer,
        CreateSyncObjects,
        FindSupportedFormat,
        CreatePipelineLayout,

        AllocateCommandBuffers,
        BeginCommandBuffer,
        EndCommandBuffer,
        AcquireNextImage,
        SubmitCommandBuffers,

        __Count__,
        __Max__ = 99
    }; // !Error

    struct Result {
        StageError stage_error;
        Error error_code;
    };
} // namespace hi