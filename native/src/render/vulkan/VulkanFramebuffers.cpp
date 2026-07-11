#include "VulkanFramebuffers.h"

#include "VulkanErrors.h"

#include <array>

namespace wbwwb::vulkan {

void VulkanFramebuffers::Create(VkDevice device,
                                VkRenderPass renderPass,
                                const std::vector<VkImageView>& swapchainImageViews,
                                VkExtent2D swapchainExtent) {
    device_ = device;
    framebuffers_.resize(swapchainImageViews.size());

    for (std::size_t i = 0; i < swapchainImageViews.size(); ++i) {
        std::array<VkImageView, 1> attachments = {swapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        CheckVk(vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &framebuffers_[i]),
                "creating Vulkan framebuffer");
    }
}

void VulkanFramebuffers::Destroy() {
    for (VkFramebuffer framebuffer : framebuffers_) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
    }
    framebuffers_.clear();
    device_ = VK_NULL_HANDLE;
}

} // namespace wbwwb::vulkan
