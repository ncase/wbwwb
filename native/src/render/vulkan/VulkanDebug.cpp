#include "VulkanDebug.h"

#include "VulkanErrors.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

namespace wbwwb::vulkan {
namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[Vulkan validation] " << callbackData->pMessage << '\n';
    }
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debugMessenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func == nullptr) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return func(instance, createInfo, allocator, debugMessenger);
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* allocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, allocator);
    }
}

} // namespace

const std::vector<const char*>& VulkanDebug::ValidationLayers() {
    static const std::vector<const char*> layers = {"VK_LAYER_KHRONOS_validation"};
    return layers;
}

bool VulkanDebug::CheckValidationLayerSupport() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : ValidationLayers()) {
        bool found = false;
        for (const auto& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

void VulkanDebug::PopulateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

void VulkanDebug::Create(VkInstance instance, bool enabled) {
    instance_ = instance;
    enabled_ = enabled;

    if (!enabled_) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateCreateInfo(createInfo);
    CheckVk(CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &messenger_),
            "creating Vulkan debug messenger");
}

void VulkanDebug::Destroy() {
    if (messenger_ != VK_NULL_HANDLE) {
        DestroyDebugUtilsMessengerEXT(instance_, messenger_, nullptr);
        messenger_ = VK_NULL_HANDLE;
    }
    instance_ = VK_NULL_HANDLE;
    enabled_ = false;
}

} // namespace wbwwb::vulkan
