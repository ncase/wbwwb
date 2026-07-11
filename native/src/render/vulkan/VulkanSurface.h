#pragma once

// Vulkan platform surface ownership.

#include <vulkan/vulkan.h>

namespace wbwwb::vulkan {

class VulkanSurface final {
public:
    void SetExisting(VkInstance instance, VkSurfaceKHR surface, bool takeOwnership);
    void CreateWin32(VkInstance instance, void* hinstance, void* hwnd);
    void Destroy();

    [[nodiscard]] VkSurfaceKHR Get() const noexcept { return surface_; }
    [[nodiscard]] bool IsValid() const noexcept { return surface_ != VK_NULL_HANDLE; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    bool ownsSurface_ = false;
};

} // namespace wbwwb::vulkan
