#pragma once

// Vulkan physical/logical device and queue selection.

#include "VulkanTypes.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace wbwwb::vulkan {

class VulkanDevice final {
public:
    void Create(VkInstance instance, VkSurfaceKHR surface, bool validationEnabled);
    void Destroy();

    [[nodiscard]] VkPhysicalDevice Physical() const noexcept { return physicalDevice_; }
    [[nodiscard]] VkDevice Logical() const noexcept { return device_; }
    [[nodiscard]] VkQueue GraphicsQueue() const noexcept { return graphicsQueue_; }
    [[nodiscard]] VkQueue PresentQueue() const noexcept { return presentQueue_; }
    [[nodiscard]] const QueueFamilyIndices& QueueFamilies() const noexcept { return queueFamilies_; }
    [[nodiscard]] bool HdrMetadataEnabled() const noexcept { return hdrMetadataEnabled_; }

    [[nodiscard]] SwapchainSupportDetails QuerySwapchainSupport(VkSurfaceKHR surface) const;
    [[nodiscard]] std::uint32_t FindMemoryType(std::uint32_t typeFilter,
                                               VkMemoryPropertyFlags properties) const;

private:
    static const std::vector<const char*>& RequiredDeviceExtensions();

    [[nodiscard]] QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    [[nodiscard]] bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    [[nodiscard]] bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    void CreateLogicalDevice(bool validationEnabled);

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    QueueFamilyIndices queueFamilies_;
    bool hdrMetadataEnabled_ = false;
};

} // namespace wbwwb::vulkan
