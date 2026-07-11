#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace wbwwb::vulkan {

class VulkanTexture final {
public:
    void CreateRGBA8(const VulkanDevice& device,
                     VkCommandPool commandPool,
                     VkQueue graphicsQueue,
                     std::uint32_t width,
                     std::uint32_t height,
                     const void* rgbaPixels,
                     VkFilter filter = VK_FILTER_LINEAR);
    void Destroy(VkDevice device);
    void SetFilter(VkDevice device, VkFilter filter);

    [[nodiscard]] VkImage Image() const noexcept { return image_; }
    [[nodiscard]] VkImageView ImageView() const noexcept { return imageView_; }
    [[nodiscard]] VkSampler Sampler() const noexcept { return sampler_; }
    [[nodiscard]] std::uint32_t Width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t Height() const noexcept { return height_; }

private:
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView imageView_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};

} // namespace wbwwb::vulkan
