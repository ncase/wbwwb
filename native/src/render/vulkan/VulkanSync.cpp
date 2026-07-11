#include "VulkanSync.h"

#include "VulkanErrors.h"

namespace wbwwb::vulkan {

void VulkanSync::Create(VkDevice device, std::uint32_t swapchainImageCount) {
    device_ = device;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (std::uint32_t i = 0; i < kFramesInFlight; ++i) {
        CheckVk(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailable_[i]),
                "creating Vulkan image-available semaphore");
        CheckVk(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]),
                "creating Vulkan in-flight fence");
    }

    renderFinished_.resize(swapchainImageCount);
    for (std::uint32_t i = 0; i < swapchainImageCount; ++i) {
        CheckVk(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinished_[i]),
                "creating Vulkan render-finished semaphore");
    }
}

void VulkanSync::Destroy() {
    for (std::uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (imageAvailable_[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, imageAvailable_[i], nullptr);
            imageAvailable_[i] = VK_NULL_HANDLE;
        }
        if (inFlightFences_[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device_, inFlightFences_[i], nullptr);
            inFlightFences_[i] = VK_NULL_HANDLE;
        }
    }

    for (VkSemaphore semaphore : renderFinished_) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }
    }
    renderFinished_.clear();

    device_ = VK_NULL_HANDLE;
}

} // namespace wbwwb::vulkan
