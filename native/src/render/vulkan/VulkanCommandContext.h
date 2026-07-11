#pragma once

// Vulkan command-pool and per-frame command-buffer ownership.

#include "VulkanTypes.h"

#include <vulkan/vulkan.h>

#include <array>

namespace wbwwb::vulkan {

class VulkanCommandContext final {
public:
    void Create(VkDevice device, std::uint32_t graphicsQueueFamily);
    void Destroy();

    [[nodiscard]] VkCommandPool Pool() const noexcept { return commandPool_; }
    [[nodiscard]] VkCommandBuffer Buffer(std::uint32_t frameIndex) const noexcept { return commandBuffers_[frameIndex]; }

    void BeginFrame(std::uint32_t frameIndex);
    void EndFrame(std::uint32_t frameIndex);

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, kFramesInFlight> commandBuffers_{};
};

} // namespace wbwwb::vulkan
