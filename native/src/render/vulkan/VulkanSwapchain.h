#pragma once

// Vulkan SDR/HDR swapchain ownership.

#include "VulkanDevice.h"
#include "VulkanTypes.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace wbwwb::vulkan {

class VulkanSwapchain final {
public:
    void Create(const VulkanDevice& device,
                VkSurfaceKHR surface,
                std::uint32_t requestedWidth,
                std::uint32_t requestedHeight,
                VulkanHdrMode hdrMode,
                bool vsync);
    void Destroy();

    [[nodiscard]] VkSwapchainKHR Get() const noexcept { return swapchain_; }
    [[nodiscard]] VkFormat ImageFormat() const noexcept { return imageFormat_; }
    [[nodiscard]] VkColorSpaceKHR ColorSpace() const noexcept { return colorSpace_; }
    [[nodiscard]] VkPresentModeKHR PresentMode() const noexcept { return presentMode_; }
    [[nodiscard]] bool HdrAvailable() const noexcept { return hdrAvailable_; }
    [[nodiscard]] bool HdrActive() const noexcept { return hdrActive_; }
    [[nodiscard]] VkExtent2D Extent() const noexcept { return extent_; }
    [[nodiscard]] const std::vector<VkImage>& Images() const noexcept { return images_; }
    [[nodiscard]] const std::vector<VkImageView>& ImageViews() const noexcept { return imageViews_; }

private:
    [[nodiscard]] static VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats,
                                                                VulkanHdrMode hdrMode,
                                                                bool& hdrAvailable,
                                                                bool& hdrActive);
    [[nodiscard]] static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes,
                                                            bool vsync);
    [[nodiscard]] static VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                                 std::uint32_t requestedWidth,
                                                 std::uint32_t requestedHeight);
    void CreateImageViews();

    VkDevice device_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    VkFormat imageFormat_ = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace_ = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D extent_{};
    bool hdrAvailable_ = false;
    bool hdrActive_ = false;
};

} // namespace wbwwb::vulkan
