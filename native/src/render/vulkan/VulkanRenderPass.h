#pragma once

// Vulkan render-pass ownership.

#include <vulkan/vulkan.h>

namespace wbwwb::vulkan {

class VulkanRenderPass final {
public:
    void Create(VkDevice device,
                VkFormat imageFormat,
                VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    void Destroy();

    [[nodiscard]] VkRenderPass Get() const noexcept { return renderPass_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
};

} // namespace wbwwb::vulkan
