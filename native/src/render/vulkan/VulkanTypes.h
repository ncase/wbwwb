#pragma once

// Shared Vulkan backend types.

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wbwwb::vulkan {

constexpr std::uint32_t kFramesInFlight = 2;

enum class VulkanHdrMode {
    Off,
    Auto,
    On
};

enum class VulkanUiTestMode {
    None,
    Menu,
    Settings,
    SharedGameBoundary
};

struct VulkanUiRowState {
    std::string labelKey;
    std::string value;
    bool valueIsLocalizationKey = false;
    bool slider = false;
    float sliderValue = 0.0f;
    bool selected = false;
    bool hovered = false;
    bool enabled = true;
};

struct VulkanMenuRuntimeState {
    float playHoverScale = 1.0f;
    float settingsHoverScale = 1.0f;
    int selected = 0;
    bool playHovered = false;
    bool settingsHovered = false;
};

struct VulkanSettingsRuntimeState {
    int section = 0;
    float scrollOffset = 0.0f;
    std::vector<VulkanUiRowState> rows;
};

struct VulkanUiRuntimeState {
    VulkanUiTestMode mode = VulkanUiTestMode::None;
    VulkanMenuRuntimeState menu;
    VulkanSettingsRuntimeState settings;
};

struct QueueFamilyIndices {
    std::optional<std::uint32_t> graphicsFamily;
    std::optional<std::uint32_t> presentFamily;

    [[nodiscard]] bool IsComplete() const noexcept {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanBackendConfig {
    std::string applicationName = "Native WBWWB C++ Port";
    std::uint32_t windowWidth = 1280;
    std::uint32_t windowHeight = 720;
    std::uint32_t renderWidth = 960;
    std::uint32_t renderHeight = 540;

    // Enable validation layers. VulkanInstance also disables this automatically if layers are unavailable.
    bool enableValidation = true;

    // Backend clear colour used before scene drawing.
    std::array<float, 4> clearColor{0.03f, 0.03f, 0.04f, 1.0f};

    // Player-selected swapchain output mode. Unsupported HDR requests fall back to SDR.
    VulkanHdrMode hdrMode = VulkanHdrMode::Off;
    bool vsync = true;

    // Draws a small deterministic 2D overlay for backend smoke tests.
    bool drawSmokeGeometry = false;

    // Optional deterministic asset for renderer smoke tests.
    bool drawGameAssetSmoke = false;
    std::filesystem::path smokeTexturePath;

    // Optional real UI assets used by focused renderer tests.
    VulkanUiTestMode uiTestMode = VulkanUiTestMode::None;
    std::filesystem::path assetRoot;
    std::filesystem::path fontPath;
    std::filesystem::path localizationPath;
    bool outputLinearFilter = true;
    std::uint32_t msaaSamples = 1;

    // Directory containing compiled SPIR-V files for the active Vulkan renderer.
    std::filesystem::path shaderDirectory;

    // Optional engine-provided surface. If supplied, win32Hinstance/win32Hwnd are ignored.
    VkSurfaceKHR existingSurface = VK_NULL_HANDLE;
    bool ownsExistingSurface = false;

    // Win32 surface creation data. Store as void* to avoid leaking windows.h into generic headers.
    void* win32Hinstance = nullptr;
    void* win32Hwnd = nullptr;

    // Add platform/window-system extensions here if the app already knows them, for example from GLFW/SDL.
    std::vector<const char*> requiredInstanceExtensions;
};

struct VulkanBackendStats {
    std::uint32_t swapchainImageCount = 0;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR swapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D swapchainExtent{};
    VkExtent2D renderTargetExtent{};
    bool hdrSwapchainAvailable = false;
    bool hdrActive = false;
    bool hdrSystemEnabled = false;
    bool hdrMetadataAvailable = false;
    float hdrPaperWhiteNits = 203.0f;
    float hdrPeakNits = 1000.0f;
};

} // namespace wbwwb::vulkan
