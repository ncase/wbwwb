#pragma once

// Vulkan HDR surface capability probing.

#include <vulkan/vulkan.h>

namespace wbwwb::vulkan {

struct VulkanHDRCapabilities {
    bool hdrMetadataExtensionAvailable = false;
    bool hdrSwapchainColorSpaceAvailable = false;
    VkColorSpaceKHR preferredHdrColorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
};

struct VulkanDisplayHDRState {
    bool hdrSupported = false;
    bool hdrEnabled = false;
    float sdrWhiteLevelNits = 203.0f;
};

class VulkanHDR final {
public:
    // Probe only. This does not enable HDR and does not claim HDR output.
    [[nodiscard]] VulkanHDRCapabilities Probe(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const;
    [[nodiscard]] VulkanDisplayHDRState QueryDisplayState(void* win32Hwnd) const;
};

} // namespace wbwwb::vulkan
