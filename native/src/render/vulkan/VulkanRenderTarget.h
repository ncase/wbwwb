#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace wbwwb::vulkan {

class VulkanRenderTarget final {
public:
    void Create(const VulkanDevice& device,
                VkRenderPass renderPass,
                std::uint32_t width,
                std::uint32_t height,
                VkFormat format,
                VkFilter filter,
                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    void Destroy(VkDevice device);
    void SetFilter(VkDevice device, VkFilter filter);

    [[nodiscard]] VkImage Image() const noexcept { return image_; }
    [[nodiscard]] VkImageView ImageView() const noexcept { return imageView_; }
    [[nodiscard]] VkSampler Sampler() const noexcept { return sampler_; }
    [[nodiscard]] VkFramebuffer Framebuffer() const noexcept { return framebuffer_; }
    [[nodiscard]] VkExtent2D Extent() const noexcept { return VkExtent2D{width_, height_}; }
    [[nodiscard]] VkFormat Format() const noexcept { return format_; }
    [[nodiscard]] VkSampleCountFlagBits Samples() const noexcept { return samples_; }

private:
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView imageView_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    VkImage msaaImage_ = VK_NULL_HANDLE;
    VkDeviceMemory msaaMemory_ = VK_NULL_HANDLE;
    VkImageView msaaImageView_ = VK_NULL_HANDLE;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits samples_ = VK_SAMPLE_COUNT_1_BIT;
};

} // namespace wbwwb::vulkan
