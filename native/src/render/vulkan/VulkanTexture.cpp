#include "VulkanTexture.h"

#include "VulkanErrors.h"

#include <cstring>
#include <stdexcept>

namespace wbwwb::vulkan {

namespace {

VkSampler CreateTextureSampler(VkDevice device, VkFilter filter) {
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
            "creating Vulkan texture sampler");
    return sampler;
}

void CreateBuffer(const VulkanDevice& device,
                  VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer& buffer,
                  VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CheckVk(vkCreateBuffer(device.Logical(), &bufferInfo, nullptr, &buffer),
            "creating Vulkan buffer");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device.Logical(), buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(memoryRequirements.memoryTypeBits, properties);

    CheckVk(vkAllocateMemory(device.Logical(), &allocInfo, nullptr, &memory),
            "allocating Vulkan buffer memory");
    CheckVk(vkBindBufferMemory(device.Logical(), buffer, memory, 0),
            "binding Vulkan buffer memory");
}

VkCommandBuffer BeginSingleUseCommands(VkDevice device, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CheckVk(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer),
            "allocating one-shot Vulkan command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CheckVk(vkBeginCommandBuffer(commandBuffer, &beginInfo),
            "beginning one-shot Vulkan command buffer");
    return commandBuffer;
}

void EndSingleUseCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer) {
    CheckVk(vkEndCommandBuffer(commandBuffer), "ending one-shot Vulkan command buffer");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    CheckVk(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE),
            "submitting one-shot Vulkan command buffer");
    CheckVk(vkQueueWaitIdle(queue), "waiting for one-shot Vulkan command buffer");

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void TransitionImageLayout(VkDevice device,
                           VkCommandPool commandPool,
                           VkQueue queue,
                           VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = BeginSingleUseCommands(device, commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("Unsupported Vulkan texture layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage,
                         destinationStage,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    EndSingleUseCommands(device, commandPool, queue, commandBuffer);
}

void CopyBufferToImage(VkDevice device,
                       VkCommandPool commandPool,
                       VkQueue queue,
                       VkBuffer buffer,
                       VkImage image,
                       std::uint32_t width,
                       std::uint32_t height) {
    VkCommandBuffer commandBuffer = BeginSingleUseCommands(device, commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleUseCommands(device, commandPool, queue, commandBuffer);
}

} // namespace

void VulkanTexture::CreateRGBA8(const VulkanDevice& device,
                                VkCommandPool commandPool,
                                VkQueue graphicsQueue,
                                std::uint32_t width,
                                std::uint32_t height,
                                const void* rgbaPixels,
                                VkFilter filter) {
    if (width == 0 || height == 0 || rgbaPixels == nullptr) {
        throw std::runtime_error("Cannot create a Vulkan texture from empty RGBA data");
    }
    if (image_ != VK_NULL_HANDLE) {
        throw std::runtime_error("VulkanTexture is already initialized");
    }

    const VkDevice vkDevice = device.Logical();
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    CreateBuffer(device,
                 imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingMemory);

    void* mapped = nullptr;
    CheckVk(vkMapMemory(vkDevice, stagingMemory, 0, imageSize, 0, &mapped),
            "mapping Vulkan texture staging memory");
    std::memcpy(mapped, rgbaPixels, static_cast<std::size_t>(imageSize));
    vkUnmapMemory(vkDevice, stagingMemory);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CheckVk(vkCreateImage(vkDevice, &imageInfo, nullptr, &image_),
            "creating Vulkan texture image");

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(vkDevice, image_, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(
        memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CheckVk(vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory_),
            "allocating Vulkan texture memory");
    CheckVk(vkBindImageMemory(vkDevice, image_, memory_, 0),
            "binding Vulkan texture memory");

    TransitionImageLayout(vkDevice,
                          commandPool,
                          graphicsQueue,
                          image_,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(vkDevice, commandPool, graphicsQueue, stagingBuffer, image_, width, height);
    TransitionImageLayout(vkDevice,
                          commandPool,
                          graphicsQueue,
                          image_,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
    vkFreeMemory(vkDevice, stagingMemory, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    CheckVk(vkCreateImageView(vkDevice, &viewInfo, nullptr, &imageView_),
            "creating Vulkan texture image view");

    sampler_ = CreateTextureSampler(vkDevice, filter);

    width_ = width;
    height_ = height;
}

void VulkanTexture::SetFilter(VkDevice device, VkFilter filter) {
    if (imageView_ == VK_NULL_HANDLE) {
        return;
    }
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler_, nullptr);
    }
    sampler_ = CreateTextureSampler(device, filter);
}

void VulkanTexture::Destroy(VkDevice device) {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
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
}

} // namespace wbwwb::vulkan
