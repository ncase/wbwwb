#include "VulkanSurface.h"

#include "VulkanErrors.h"

#include <stdexcept>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace wbwwb::vulkan {

void VulkanSurface::SetExisting(VkInstance instance, VkSurfaceKHR surface, bool takeOwnership) {
    if (surface == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot attach a null Vulkan surface");
    }
    instance_ = instance;
    surface_ = surface;
    ownsSurface_ = takeOwnership;
}

void VulkanSurface::CreateWin32(VkInstance instance, void* hinstance, void* hwnd) {
#ifdef _WIN32
    if (hinstance == nullptr || hwnd == nullptr) {
        throw std::runtime_error("Win32 Vulkan surface requires both HINSTANCE and HWND");
    }

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = static_cast<HINSTANCE>(hinstance);
    createInfo.hwnd = static_cast<HWND>(hwnd);

    CheckVk(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface_),
            "creating Win32 Vulkan surface");
    instance_ = instance;
    ownsSurface_ = true;
#else
    (void)instance;
    (void)hinstance;
    (void)hwnd;
    throw std::runtime_error("Win32 Vulkan surface requested on a non-Windows build");
#endif
}

void VulkanSurface::Destroy() {
    if (ownsSurface_ && surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }
    instance_ = VK_NULL_HANDLE;
    surface_ = VK_NULL_HANDLE;
    ownsSurface_ = false;
}

} // namespace wbwwb::vulkan
