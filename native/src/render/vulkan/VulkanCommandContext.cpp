#include "VulkanCommandContext.h"

#include "VulkanErrors.h"

namespace wbwwb::vulkan {

void VulkanCommandContext::Create(VkDevice device, std::uint32_t graphicsQueueFamily) {
    device_ = device;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;

    CheckVk(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_),
            "creating Vulkan command pool");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = kFramesInFlight;

    CheckVk(vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()),
            "allocating Vulkan command buffers");
}

void VulkanCommandContext::BeginFrame(std::uint32_t frameIndex) {
    VkCommandBuffer commandBuffer = commandBuffers_[frameIndex];
    CheckVk(vkResetCommandBuffer(commandBuffer, 0), "resetting Vulkan command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CheckVk(vkBeginCommandBuffer(commandBuffer, &beginInfo), "beginning Vulkan command buffer");
}

void VulkanCommandContext::EndFrame(std::uint32_t frameIndex) {
    CheckVk(vkEndCommandBuffer(commandBuffers_[frameIndex]), "ending Vulkan command buffer");
}

void VulkanCommandContext::Destroy() {
    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
    commandBuffers_ = {};
    device_ = VK_NULL_HANDLE;
}

} // namespace wbwwb::vulkan
