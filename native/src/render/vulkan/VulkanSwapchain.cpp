#include "VulkanSwapchain.h"

#include "VulkanErrors.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace wbwwb::vulkan {

namespace {

bool IsHdrColorSpace(VkColorSpaceKHR colorSpace) {
    if (colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
        return true;
    }
    if (colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
        return true;
    }
    return false;
}

bool IsHdrFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return true;
    default:
        return false;
    }
}

bool IsHdrSurfaceFormat(const VkSurfaceFormatKHR& format) {
    return IsHdrColorSpace(format.colorSpace) && IsHdrFormat(format.format);
}

} // namespace

VkSurfaceFormatKHR VulkanSwapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats,
                                                        VulkanHdrMode hdrMode,
                                                        bool& hdrAvailable,
                                                        bool& hdrActive) {
    hdrAvailable = std::any_of(formats.begin(), formats.end(), IsHdrSurfaceFormat);
    hdrActive = false;

    if (hdrMode != VulkanHdrMode::Off) {
        for (const auto& availableFormat : formats) {
            if (availableFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                availableFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
                hdrActive = true;
                return availableFormat;
            }
        }
        for (const auto& availableFormat : formats) {
            if ((availableFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 ||
                 availableFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32) &&
                availableFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
                hdrActive = true;
                return availableFormat;
            }
        }
        for (const auto& availableFormat : formats) {
            if (IsHdrSurfaceFormat(availableFormat)) {
                hdrActive = true;
                return availableFormat;
            }
        }
    }

    if (formats.size() == 1 && formats.front().format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    if (hdrMode == VulkanHdrMode::Off) {
        for (const auto& availableFormat : formats) {
            if (availableFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                availableFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
                return availableFormat;
            }
        }
    }
    return formats.front();
}

VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes, bool vsync) {
    if (vsync) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                         std::uint32_t requestedWidth,
                                         std::uint32_t requestedHeight) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent{requestedWidth, requestedHeight};
    actualExtent.width = std::clamp(actualExtent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
                                     capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);
    return actualExtent;
}

void VulkanSwapchain::Create(const VulkanDevice& device,
                             VkSurfaceKHR surface,
                             std::uint32_t requestedWidth,
                             std::uint32_t requestedHeight,
                             VulkanHdrMode hdrMode,
                             bool vsync) {
    device_ = device.Logical();

    SwapchainSupportDetails swapchainSupport = device.QuerySwapchainSupport(surface);
    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapchainSupport.formats, hdrMode, hdrAvailable_, hdrActive_);
    VkPresentModeKHR presentMode = ChoosePresentMode(swapchainSupport.presentModes, vsync);
    VkExtent2D extent = ChooseExtent(swapchainSupport.capabilities, requestedWidth, requestedHeight);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const QueueFamilyIndices& indices = device.QueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    CheckVk(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_),
            "creating Vulkan swapchain");

    CheckVk(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr),
            "querying Vulkan swapchain image count");
    images_.resize(imageCount);
    CheckVk(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, images_.data()),
            "querying Vulkan swapchain images");

    imageFormat_ = surfaceFormat.format;
    colorSpace_ = surfaceFormat.colorSpace;
    presentMode_ = presentMode;
    extent_ = extent;

    CreateImageViews();
}

void VulkanSwapchain::CreateImageViews() {
    imageViews_.resize(images_.size());

    for (std::size_t i = 0; i < images_.size(); ++i) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images_[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = imageFormat_;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        CheckVk(vkCreateImageView(device_, &createInfo, nullptr, &imageViews_[i]),
                "creating Vulkan swapchain image view");
    }
}

void VulkanSwapchain::Destroy() {
    for (VkImageView imageView : imageViews_) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, imageView, nullptr);
        }
    }
    imageViews_.clear();
    images_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    imageFormat_ = VK_FORMAT_UNDEFINED;
    colorSpace_ = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
    extent_ = {};
    hdrAvailable_ = false;
    hdrActive_ = false;
    device_ = VK_NULL_HANDLE;
}

} // namespace wbwwb::vulkan
