#pragma once

// Vulkan validation debug messenger.

#include <vulkan/vulkan.h>

#include <vector>

namespace wbwwb::vulkan {

class VulkanDebug final {
public:
    static const std::vector<const char*>& ValidationLayers();
    static bool CheckValidationLayerSupport();
    static void PopulateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void Create(VkInstance instance, bool enabled);
    void Destroy();

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
    bool enabled_ = false;
};

} // namespace wbwwb::vulkan
