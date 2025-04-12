#include "device.hpp"
#include "../higui/higui_types.hpp"

namespace hi {
    // --- Destructor
    
    EngineDevice::~EngineDevice() noexcept {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        vkDestroyDevice(device_, nullptr);
#ifndef NDEBUG
        if constexpr (vk_enable_validation_layers)
            debug::destroy_utils_messenger_ext(instance_, debug_messenger_, nullptr);
#endif
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        vkDestroyInstance(instance_, nullptr);
    }

    // --- Instance creation
    
    
    Error EngineDevice::create_instance() {
#ifndef NDEBUG // --------- Debug only -------
        if (!check_validation_layer_support())
            return Error::ValidationLayers;
#endif // ------------------------------------

        VkApplicationInfo app_info{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = VK_API_VERSION_1_0,
        };

#ifndef NDEBUG // --------- Debug only -------
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        debug::populate_messenger_create_info(debug_create_info);
#endif // ------------------------------------

        VkInstanceCreateInfo create_info{ .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    #ifndef NDEBUG // ---- DEBUG ----
            .pNext = &debug_create_info,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = static_cast<uint32_t>(sizeof(validation_layers_) / sizeof(validation_layers_[0])),
            .ppEnabledLayerNames = validation_layers_,
    #else // ------------- RELEASE ----
            .pNext = nullptr,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
    #endif // -------------------------
            .enabledExtensionCount = static_cast<uint32_t>(sizeof(vk_required_extensions) / sizeof(vk_required_extensions[0])),
            .ppEnabledExtensionNames = vk_required_extensions,
        }; // !create_info

        if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
            return Error::VulkanInstance;
        return Error::None;
    }

    // --- Debug messenger (debug-mode only)


#ifndef NDEBUG
    

    Error EngineDevice::setup_debug_messenger() {
        VkDebugUtilsMessengerCreateInfoEXT create_info{};
        debug::populate_messenger_create_info(create_info);
        if (debug::create_utils_messenger_ext(instance_, &create_info, nullptr, &debug_messenger_) != VK_SUCCESS)
            // Failed to set up debug messenger
            return Error::DebugMessenger;
        return Error::None; // OK
    }
#endif


    // --- Create: `surface`, `physical device`, `logical device` and `command pool`


    Error EngineDevice::create_surface() {
        return window_.create_vulkan_surface(instance_, &surface_) == VK_SUCCESS ?
            Error::None :
            Error::SurfaceKhr;
    }

    Error EngineDevice::pick_physical_device() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance_, &count, nullptr);
        if (count == 0) return
            Error::VulkanSupport;

        size_t devices_size = sizeof(VkPhysicalDevice) * count;
        VkPhysicalDevice* devices = static_cast<VkPhysicalDevice*>(
            hi::alloc(devices_size)); // Allocation: Check.
        if (!devices)
            return Error::InternalMemoryAlloc;

        vkEnumeratePhysicalDevices(instance_, &count, devices);
        for (uint32_t i = 0; i < count; ++i) {
            if (is_device_suitable(devices[i])) {
                physical_device_ = devices[i];
                break;
            }
        }
        hi::free(devices, devices_size); // Allocation: Free.

        if (physical_device_ == VK_NULL_HANDLE)
            return Error::SuitableGpu;
        vkGetPhysicalDeviceProperties(physical_device_, &properties_);
#ifndef NDEBUG
        debug::print_physical_device(properties_);
#endif // NDEBUG
        return Error::None;
    }

    Error EngineDevice::create_logical_device() {
        queue_family_indices indices = find_queue_families(physical_device_);
        uint32_t unique_families[2];
        uint32_t count = 0;
        if (indices.graphics_family_has_value) unique_families[count++] = indices.graphics_family;
        if (indices.present_family_has_value && indices.present_family != indices.graphics_family)
            unique_families[count++] = indices.present_family;

        size_t queue_create_infos_size = sizeof(VkDeviceQueueCreateInfo) * count;
        VkDeviceQueueCreateInfo* queue_create_infos = static_cast<VkDeviceQueueCreateInfo*>(
            hi::alloc(queue_create_infos_size)); // Allocation: Check.
        if (!queue_create_infos)
            return Error::InternalMemoryAlloc;

        float queue_priority = 1.0f;
        for (uint32_t i = 0; i < count; ++i) {
            queue_create_infos[i] = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = unique_families[i],
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            };
        }
        VkPhysicalDeviceFeatures device_features = {
            .samplerAnisotropy = VK_TRUE
        };
        VkDeviceCreateInfo create_info = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = count,
            .pQueueCreateInfos = queue_create_infos,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = device_extensions_,
            .pEnabledFeatures = &device_features,
        };
        VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
        hi::free(queue_create_infos, queue_create_infos_size); // Allocation: Free.

        if (result != VK_SUCCESS)
            return Error::LogicalDevice;
        vkGetDeviceQueue(device_, indices.graphics_family, 0, &graphics_queue_);
        vkGetDeviceQueue(device_, indices.present_family, 0, &present_queue_);
        return Error::None;
    }

    Error EngineDevice::create_command_pool() {
        queue_family_indices queue_indices = find_queue_families(physical_device_);
        VkCommandPoolCreateInfo pool_info{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queue_indices.graphics_family,
        };
        if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS)
            return Error::CommandPool; // Failed to create command pool
        return Error::None;
    }


    // --- Helper function


    bool EngineDevice::is_device_suitable(VkPhysicalDevice device) {
        queue_family_indices indices = find_queue_families(device);
        bool extensions_supported = check_device_extension_support(device);
        bool swapchain_adequate = false;
        if (extensions_supported) {
            swap_chain_support_details swapchain_support = query_swapchain_support(device);
            swapchain_adequate =
                swapchain_support.format_count != 0
                && swapchain_support.present_modes != 0;
        }
        VkPhysicalDeviceFeatures supported_features;
        vkGetPhysicalDeviceFeatures(device, &supported_features);
        return indices.is_complete() && extensions_supported && swapchain_adequate &&
            supported_features.samplerAnisotropy;
    }

#ifndef NDEBUG
    bool EngineDevice::check_validation_layer_support() {
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        size_t layers_size = sizeof(VkLayerProperties) * count;
        VkLayerProperties* layers = static_cast<VkLayerProperties*>(
            hi::alloc(layers_size)); // Allocation: Check.
        if (!layers)
            return false;
        vkEnumerateInstanceLayerProperties(&count, layers);
        bool result = false;
        for (uint32_t i = 0; i < count; ++i) {
            if (str_equal(validation_layers_[0], layers[i].layerName)) {
                result = true;
                break;
            }
        }
        hi::free(layers, layers_size); // Allocation: Free.
        return result;
    }
#endif

    queue_family_indices EngineDevice::find_queue_families(VkPhysicalDevice device) {
        queue_family_indices indices{};
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

        size_t props_size = sizeof(VkQueueFamilyProperties) * count;
        VkQueueFamilyProperties* props = static_cast<VkQueueFamilyProperties*>(
            hi::alloc(props_size)); // Allocation: Check.
        if (!props)
            return indices;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props);
        for (uint32_t i = 0; i < count; ++i) {
            if (props[i].queueCount > 0 && (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                indices.graphics_family = i;
                indices.graphics_family_has_value = true;
            }
            VkBool32 present;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present);
            if (props[i].queueCount > 0 && present) {
                indices.present_family = i;
                indices.present_family_has_value = true;
            }
            if (indices.is_complete()) break;
        }
        hi::free(props, props_size); // Allocation: Free.
        return indices;
    }

    bool EngineDevice::check_device_extension_support(VkPhysicalDevice device) {
        uint32_t ext_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);

        size_t                 available_size = sizeof(VkExtensionProperties) * ext_count;
        VkExtensionProperties* available = static_cast<VkExtensionProperties*>(
            hi::alloc(available_size)); // Allocation: Check.
        if (!available)
            return false;

        vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, available);

        bool found[1] = {};
        for (uint32_t i = 0; i < ext_count; ++i) {
            if (str_equal(device_extensions_[0], available[i].extensionName))
                found[0] = true;
        }
        hi::free(available, available_size); // Allocation: Free.
        return found[0];
    }

    swap_chain_support_details EngineDevice::query_swapchain_support(VkPhysicalDevice device) {
        swap_chain_support_details details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
        if (format_count != 0) {
            if (format_count > sizeof(details.formats) / sizeof(details.formats[0])) {
                hi::window::show_str_error("Swapchain format", "Limit has been exceeded");
                format_count = sizeof(details.formats) / sizeof(details.formats[0]);
            }
            details.format_count = format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats);
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
        if (present_mode_count != 0) {
            if (present_mode_count > sizeof(details.present_modes) / sizeof(details.present_modes[0])) {
                hi::window::show_str_error("Swapchain present mode", "Limit has been exceeded");
                present_mode_count = sizeof(details.present_modes) / sizeof(details.present_modes[0]);
            }
            details.present_mode_count = present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes);
        }

        return details;
    }

    // check the result, `UINT32_MAX` means error: Error::SuitableMemoryType
    uint32_t EngineDevice::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) noexcept {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) &&
                (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        // Failed to find suitable memory type
        return UINT32_MAX;
    }

    Error EngineDevice::create_buffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& buffer_memory) noexcept {

        VkBufferCreateInfo buf_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        if (vkCreateBuffer(device_, &buf_info, nullptr, &buffer) != VK_SUCCESS) {
            return Error::BufferInit;
        }

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device_, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_requirements.size,
            .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties),
        };

        if (alloc_info.memoryTypeIndex == UINT32_MAX)
            return Error::SuitableMemoryType;

        if (vkAllocateMemory(device_, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
            return Error::BufferAlloc;
        }

        vkBindBufferMemory(device_, buffer, buffer_memory, 0);
        return Error::None;
    }

    VkCommandBuffer EngineDevice::begin_single_time_commands() noexcept {
        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool_,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(command_buffer, &begin_info);
        return command_buffer;
    }

    void EngineDevice::end_single_time_commands(VkCommandBuffer command_buffer) noexcept {
        vkEndCommandBuffer(command_buffer);
        VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
        };
        vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphics_queue_);
        vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);
    }

    void EngineDevice::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) noexcept {
        VkCommandBuffer command_buffer = begin_single_time_commands();

        VkBufferCopy copy_region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
        end_single_time_commands(command_buffer);
    }

    void EngineDevice::copy_buffer_to_image(VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height,
        uint32_t layer_count) noexcept {
        VkCommandBuffer command_buffer = begin_single_time_commands();

        VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = layer_count
                },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { width, height, 1 },
        };
        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        end_single_time_commands(command_buffer);
    }

    Error EngineDevice::create_image_with_info(const VkImageCreateInfo& image_info,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& image_memory) noexcept {
        if (vkCreateImage(device_, &image_info, nullptr, &image) != VK_SUCCESS)
            return Error::ImageInit; // Failed to create image

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(device_, image, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_requirements.size,
            .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties),
        };

        if (alloc_info.memoryTypeIndex == UINT32_MAX)
            return Error::SuitableMemoryType;

        if (vkAllocateMemory(device_, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
            return Error::ImageAlloc; // Failed to allocate memory for image

        if (vkBindImageMemory(device_, image, image_memory, 0) != VK_SUCCESS) {
            return Error::ImageBind; // Failed to bind image memory
        }
        return Error::None;
    }
} // namespace hi