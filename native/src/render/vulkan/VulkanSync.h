#pragma once

// Vulkan frame synchronization resources.

#include "VulkanTypes.h"

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

namespace wbwwb::vulkan {

class VulkanSync final {
public:
    void Create(VkDevice device, std::uint32_t swapchainImageCount);
    void Destroy();

    [[nodiscard]] VkSemaphore ImageAvailable(std::uint32_t frameIndex) const noexcept { return imageAvailable_[frameIndex]; }
    [[nodiscard]] VkSemaphore RenderFinished(std::uint32_t imageIndex) const noexcept { return renderFinished_[imageIndex]; }
    [[nodiscard]] VkFence InFlight(std::uint32_t frameIndex) const noexcept { return inFlightFences_[frameIndex]; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    std::array<VkSemaphore, kFramesInFlight> imageAvailable_{};
    std::array<VkFence, kFramesInFlight> inFlightFences_{};
    std::vector<VkSemaphore> renderFinished_;
};

} // namespace wbwwb::vulkan
