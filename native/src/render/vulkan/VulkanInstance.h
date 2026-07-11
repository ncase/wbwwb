#pragma once

// Vulkan instance ownership and extension discovery.

#include "VulkanDebug.h"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace wbwwb::vulkan {

class VulkanInstance final {
public:
    [[nodiscard]] static bool IsExtensionAvailable(const char* extensionName);

    void Create(const std::string& applicationName,
                bool enableValidation,
                const std::vector<const char*>& requiredExtensions);
    void Destroy();

    [[nodiscard]] VkInstance Get() const noexcept { return instance_; }
    [[nodiscard]] bool ValidationEnabled() const noexcept { return validationEnabled_; }

private:
    static std::vector<const char*> BuildExtensionList(bool validationEnabled,
                                                       const std::vector<const char*>& requiredExtensions);
    static void CheckRequiredExtensions(const std::vector<const char*>& requiredExtensions);

    VkInstance instance_ = VK_NULL_HANDLE;
    VulkanDebug debug_;
    bool validationEnabled_ = false;
};

} // namespace wbwwb::vulkan
