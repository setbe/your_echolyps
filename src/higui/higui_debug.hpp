#pragma once
//#define HI_DEBUG_INFO // Uncomment to make a message box appear

#include <vulkan/vulkan.h>

#include "higui_types.hpp"
#include "higui_platform.hpp"

#ifndef NDEBUG

#include <stdio.h>

namespace hi::debug {
    static inline const char* get_device_type_name(VkPhysicalDeviceType device_type) {
        switch (device_type) {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return "other";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "integrated";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "discrete";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "virtual";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "cpu";
        default:
            return "*unknown*";
        };
    }

    inline void print_physical_device(const VkPhysicalDeviceProperties& p) {
        printf("Driver version: %d.%d.%d\n"
            "API version: %d.%d.%d\n"
            "Device name: %s\n"
            "Playing on %s videocard.\n",
            VK_VERSION_MAJOR(p.driverVersion),
            VK_VERSION_MINOR(p.driverVersion),
            VK_VERSION_PATCH(p.driverVersion),
            VK_VERSION_MAJOR(p.apiVersion),
            VK_VERSION_MINOR(p.apiVersion),
            VK_VERSION_PATCH(p.apiVersion),
            p.deviceName,
            get_device_type_name(p.deviceType));
    }


    static VKAPI_ATTR VkBool32 VKAPI_CALL callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT /*message_type*/,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
        void* /*p_user_data*/) {
        printf("[VAL LAYER] %s\n", p_callback_data->pMessage);
        return VK_FALSE;
    }

    inline void populate_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
        create_info = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = callback,
        };
    }

    inline VkResult create_utils_messenger_ext(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator,
        VkDebugUtilsMessengerEXT* p_debug_messenger) {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr) {
            return func(instance, p_create_info, p_allocator, p_debug_messenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    inline void destroy_utils_messenger_ext(VkInstance instance,
        VkDebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks* p_allocator) {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr)
            func(instance, debug_messenger, p_allocator);
    }
}
#endif