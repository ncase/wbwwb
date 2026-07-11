#include "VulkanHDR.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <vector>

namespace wbwwb::vulkan {
namespace {

struct DisplayConfigAdvancedColorInfo2 {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 value;
    DISPLAYCONFIG_COLOR_ENCODING colorEncoding;
    UINT32 bitsPerColorChannel;
    UINT32 activeColorMode;
};

constexpr DISPLAYCONFIG_DEVICE_INFO_TYPE kGetAdvancedColorInfo2 =
    static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(15);
constexpr UINT32 kHdrSupportedBit = 1u << 4u;
constexpr UINT32 kHdrUserEnabledBit = 1u << 5u;
constexpr UINT32 kAdvancedColorModeHdr = 2u;

} // namespace

VulkanHDRCapabilities VulkanHDR::Probe(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
    VulkanHDRCapabilities caps;

    uint32_t formatCount = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS ||
        formatCount == 0) {
        return caps;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()) != VK_SUCCESS) {
        return caps;
    }

    for (const auto& format : formats) {
        if (format.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
            caps.hdrSwapchainColorSpaceAvailable = true;
            caps.preferredHdrColorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
            break;
        }
        if (!caps.hdrSwapchainColorSpaceAvailable &&
            format.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
            caps.hdrSwapchainColorSpaceAvailable = true;
            caps.preferredHdrColorSpace = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
        }
    }

    uint32_t extensionCount = 0;
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) == VK_SUCCESS) {
        std::vector<VkExtensionProperties> extensions(extensionCount);
        if (vkEnumerateDeviceExtensionProperties(
                physicalDevice,
                nullptr,
                &extensionCount,
                extensions.data()) == VK_SUCCESS) {
            caps.hdrMetadataExtensionAvailable = std::any_of(
                extensions.begin(),
                extensions.end(),
                [](const VkExtensionProperties& extension) {
                    return std::strcmp(extension.extensionName, VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0;
                }
            );
        }
    }
    return caps;
}

VulkanDisplayHDRState VulkanHDR::QueryDisplayState(void* win32Hwnd) const {
    VulkanDisplayHDRState state;
    const HWND hwnd = static_cast<HWND>(win32Hwnd);
    if (!hwnd) {
        return state;
    }

    MONITORINFOEXW monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor || !GetMonitorInfoW(monitor, &monitorInfo)) {
        return state;
    }

    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    for (int attempt = 0; attempt < 3; ++attempt) {
        UINT32 pathCount = 0;
        UINT32 modeCount = 0;
        if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS) {
            return state;
        }
        paths.resize(pathCount);
        modes.resize(modeCount);
        const LONG result = QueryDisplayConfig(
            QDC_ONLY_ACTIVE_PATHS,
            &pathCount,
            paths.data(),
            &modeCount,
            modes.data(),
            nullptr
        );
        if (result == ERROR_INSUFFICIENT_BUFFER) {
            continue;
        }
        if (result != ERROR_SUCCESS) {
            return state;
        }
        paths.resize(pathCount);
        break;
    }
    if (paths.empty()) {
        return state;
    }

    for (const DISPLAYCONFIG_PATH_INFO& path : paths) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName{};
        sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        sourceName.header.size = sizeof(sourceName);
        sourceName.header.adapterId = path.sourceInfo.adapterId;
        sourceName.header.id = path.sourceInfo.id;
        if (DisplayConfigGetDeviceInfo(&sourceName.header) != ERROR_SUCCESS ||
            _wcsicmp(sourceName.viewGdiDeviceName, monitorInfo.szDevice) != 0) {
            continue;
        }

        DisplayConfigAdvancedColorInfo2 advancedColor2{};
        advancedColor2.header.type = kGetAdvancedColorInfo2;
        advancedColor2.header.size = sizeof(advancedColor2);
        advancedColor2.header.adapterId = path.targetInfo.adapterId;
        advancedColor2.header.id = path.targetInfo.id;
        if (DisplayConfigGetDeviceInfo(&advancedColor2.header) == ERROR_SUCCESS) {
            state.hdrSupported = (advancedColor2.value & kHdrSupportedBit) != 0;
            state.hdrEnabled = (advancedColor2.value & kHdrUserEnabledBit) != 0 &&
                advancedColor2.activeColorMode == kAdvancedColorModeHdr;
        } else {
            DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO advancedColor{};
            advancedColor.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
            advancedColor.header.size = sizeof(advancedColor);
            advancedColor.header.adapterId = path.targetInfo.adapterId;
            advancedColor.header.id = path.targetInfo.id;
            if (DisplayConfigGetDeviceInfo(&advancedColor.header) == ERROR_SUCCESS) {
                state.hdrSupported = advancedColor.advancedColorSupported != 0;
                state.hdrEnabled = advancedColor.advancedColorEnabled != 0;
            }
        }

        DISPLAYCONFIG_SDR_WHITE_LEVEL whiteLevel{};
        whiteLevel.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
        whiteLevel.header.size = sizeof(whiteLevel);
        whiteLevel.header.adapterId = path.targetInfo.adapterId;
        whiteLevel.header.id = path.targetInfo.id;
        if (DisplayConfigGetDeviceInfo(&whiteLevel.header) == ERROR_SUCCESS && whiteLevel.SDRWhiteLevel > 0) {
            state.sdrWhiteLevelNits = std::clamp(
                static_cast<float>(whiteLevel.SDRWhiteLevel) * 80.0f / 1000.0f,
                80.0f,
                500.0f
            );
        }
        return state;
    }

    return state;
}

} // namespace wbwwb::vulkan
