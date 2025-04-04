#include "higui/higui.hpp"
#include <vulkan/vulkan.h>

#include <Windows.h>

inline static int run() noexcept {
    hi::Surface surface(800, 600);

    if (surface.is_handler() == false)
        return -1;

    surface.set_title("Your Echolyps");

#ifdef _WIN32
    hi::clean_memory();
#endif // _WIN32


    surface.loop();

    return 0;
}

inline static void vulkan_ext() noexcept {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    VkExtensionProperties* extensions = (VkExtensionProperties*)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, extensionCount * sizeof(VkExtensionProperties));

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

    for (uint32_t i = 0; i < extensionCount; ++i) {
        MessageBoxA(nullptr, extensions[i].extensionName, "Vulkan Extension", MB_OK);
    }

    HeapFree(GetProcessHeap(), 0, extensions);
}

int main() {
    return run();
}