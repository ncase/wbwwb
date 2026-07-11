#pragma once

// Vulkan swapchain framebuffer ownership.

#include <vulkan/vulkan.h>

#include <vector>

namespace wbwwb::vulkan {

class VulkanFramebuffers final {
public:
    void Create(VkDevice device,
                VkRenderPass renderPass,
                const std::vector<VkImageView>& swapchainImageViews,
                VkExtent2D swapchainExtent);
    void Destroy();

    [[nodiscard]] const std::vector<VkFramebuffer>& Get() const noexcept { return framebuffers_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;
};

} // namespace wbwwb::vulkan
