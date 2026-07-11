#include "VulkanRenderTarget.h"

#include "VulkanErrors.h"

#include <array>
#include <stdexcept>

namespace wbwwb::vulkan {
namespace {

VkSampler CreateTargetSampler(VkDevice device, VkFilter filter) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSampler sampler = VK_NULL_HANDLE;
    CheckVk(vkCreateSampler(device, &samplerInfo, nullptr, &sampler),
            "creating Vulkan render target sampler");
    return sampler;
}

}  // namespace

void VulkanRenderTarget::Create(const VulkanDevice& device,
                                VkRenderPass renderPass,
                                std::uint32_t width,
                                std::uint32_t height,
                                VkFormat format,
                                VkFilter filter,
                                VkSampleCountFlagBits samples) {
    if (width == 0 || height == 0) {
        throw std::runtime_error("Cannot create a zero-sized Vulkan render target");
    }

    const VkDevice vkDevice = device.Logical();
    width_ = width;
    height_ = height;
    format_ = format;
    samples_ = samples;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format_;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CheckVk(vkCreateImage(vkDevice, &imageInfo, nullptr, &image_),
            "creating Vulkan render target image");

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(vkDevice, image_, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(
        memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CheckVk(vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory_),
            "allocating Vulkan render target memory");
    CheckVk(vkBindImageMemory(vkDevice, image_, memory_, 0),
            "binding Vulkan render target memory");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    CheckVk(vkCreateImageView(vkDevice, &viewInfo, nullptr, &imageView_),
            "creating Vulkan render target image view");

    sampler_ = CreateTargetSampler(vkDevice, filter);

    if (samples_ != VK_SAMPLE_COUNT_1_BIT) {
        VkImageCreateInfo msaaImageInfo = imageInfo;
        msaaImageInfo.samples = samples_;
        msaaImageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        CheckVk(vkCreateImage(vkDevice, &msaaImageInfo, nullptr, &msaaImage_),
                "creating Vulkan multisample color image");

        VkMemoryRequirements msaaMemoryRequirements{};
        vkGetImageMemoryRequirements(vkDevice, msaaImage_, &msaaMemoryRequirements);
        VkMemoryAllocateInfo msaaAllocInfo{};
        msaaAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        msaaAllocInfo.allocationSize = msaaMemoryRequirements.size;
        msaaAllocInfo.memoryTypeIndex = device.FindMemoryType(
            msaaMemoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        CheckVk(vkAllocateMemory(vkDevice, &msaaAllocInfo, nullptr, &msaaMemory_),
                "allocating Vulkan multisample color memory");
        CheckVk(vkBindImageMemory(vkDevice, msaaImage_, msaaMemory_, 0),
                "binding Vulkan multisample color memory");

        VkImageViewCreateInfo msaaViewInfo = viewInfo;
        msaaViewInfo.image = msaaImage_;
        CheckVk(vkCreateImageView(vkDevice, &msaaViewInfo, nullptr, &msaaImageView_),
                "creating Vulkan multisample color image view");
    }

    std::array<VkImageView, 2> attachments = {
        samples_ == VK_SAMPLE_COUNT_1_BIT ? imageView_ : msaaImageView_,
        imageView_
    };
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = samples_ == VK_SAMPLE_COUNT_1_BIT ? 1u : 2u;
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = width_;
    framebufferInfo.height = height_;
    framebufferInfo.layers = 1;
    CheckVk(vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &framebuffer_),
            "creating Vulkan render target framebuffer");
}

void VulkanRenderTarget::SetFilter(VkDevice device, VkFilter filter) {
    if (imageView_ == VK_NULL_HANDLE) {
        return;
    }
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler_, nullptr);
    }
    sampler_ = CreateTargetSampler(device, filter);
}

void VulkanRenderTarget::Destroy(VkDevice device) {
    if (framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    if (msaaImageView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, msaaImageView_, nullptr);
        msaaImageView_ = VK_NULL_HANDLE;
    }
    if (msaaImage_ != VK_NULL_HANDLE) {
        vkDestroyImage(device, msaaImage_, nullptr);
        msaaImage_ = VK_NULL_HANDLE;
    }
    if (msaaMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, msaaMemory_, nullptr);
        msaaMemory_ = VK_NULL_HANDLE;
    }
    if (imageView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView_, nullptr);
        imageView_ = VK_NULL_HANDLE;
    }
    if (image_ != VK_NULL_HANDLE) {
        vkDestroyImage(device, image_, nullptr);
        image_ = VK_NULL_HANDLE;
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
    width_ = 0;
    height_ = 0;
    format_ = VK_FORMAT_UNDEFINED;
    samples_ = VK_SAMPLE_COUNT_1_BIT;
}

} // namespace wbwwb::vulkan
