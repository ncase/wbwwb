#include "VulkanBackend.h"

#include "VulkanErrors.h"
#include "VulkanImageLoader.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

namespace wbwwb::vulkan {

namespace {

std::filesystem::path TexturePath(const std::filesystem::path& root, const char* relativePath) {
    return root / relativePath;
}

VulkanRenderer2D::Color Rgba(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

VkFilter ToVkFilter(wb::render::TextureFilter filter) {
    return filter == wb::render::TextureFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

VkSampleCountFlagBits ChooseSampleCount(VkPhysicalDevice physicalDevice, std::uint32_t requested) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    const VkSampleCountFlags supported = properties.limits.framebufferColorSampleCounts;
    struct Candidate {
        std::uint32_t count;
        VkSampleCountFlagBits flag;
    };
    constexpr Candidate candidates[] = {
        {16, VK_SAMPLE_COUNT_16_BIT},
        {8, VK_SAMPLE_COUNT_8_BIT},
        {4, VK_SAMPLE_COUNT_4_BIT},
        {2, VK_SAMPLE_COUNT_2_BIT}
    };
    for (const Candidate& candidate : candidates) {
        if (requested >= candidate.count && (supported & candidate.flag) != 0) {
            return candidate.flag;
        }
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

std::uint32_t SampleCountValue(VkSampleCountFlagBits samples) {
    switch (samples) {
        case VK_SAMPLE_COUNT_16_BIT: return 16;
        case VK_SAMPLE_COUNT_8_BIT: return 8;
        case VK_SAMPLE_COUNT_4_BIT: return 4;
        case VK_SAMPLE_COUNT_2_BIT: return 2;
        default: return 1;
    }
}

VkExtent2D SelectedRenderExtent(const VulkanBackendConfig& config) {
    return {
        std::max(1u, config.renderWidth),
        std::max(1u, config.renderHeight)
    };
}

float OutputEncodingFor(VkFormat format, VkColorSpaceKHR colorSpace, bool hdrActive) {
    if (!hdrActive) {
        const bool linearOutput = colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT ||
            format == VK_FORMAT_B8G8R8A8_SRGB ||
            format == VK_FORMAT_R8G8B8A8_SRGB;
        return linearOutput ? 3.0f : 0.0f;
    }
    if (colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
        return 1.0f;
    }
    return 2.0f;
}

VulkanRenderer2D::Color ToVulkanColor(wb::render::Color color, float alpha = 1.0f) {
    const float a = std::clamp(alpha, 0.0f, 1.0f) * (static_cast<float>(color.a) / 255.0f);
    return {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        a
    };
}

VulkanRenderer2D::SourceRect ToVulkanSource(wb::render::Rect rect) {
    return {rect.x, rect.y, rect.width, rect.height};
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

void TransitionColorImageLayout(VkCommandBuffer commandBuffer,
                                VkImage image,
                                VkImageLayout oldLayout,
                                VkImageLayout newLayout,
                                VkAccessFlags srcAccess,
                                VkAccessFlags dstAccess,
                                VkPipelineStageFlags srcStage,
                                VkPipelineStageFlags dstStage) {
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
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(commandBuffer,
                         srcStage,
                         dstStage,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);
}

bool IsRgbaFormat(VkFormat format) {
    return format == VK_FORMAT_R8G8B8A8_UNORM ||
           format == VK_FORMAT_R8G8B8A8_SRGB;
}

bool IsBgraFormat(VkFormat format) {
    return format == VK_FORMAT_B8G8R8A8_UNORM ||
           format == VK_FORMAT_B8G8R8A8_SRGB;
}

} // namespace

VulkanBackend::~VulkanBackend() {
    Shutdown();
}

std::vector<const char*> VulkanBackend::BuildRequiredInstanceExtensions(const VulkanBackendConfig& config) const {
    std::vector<const char*> extensions = config.requiredInstanceExtensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
    if (config.existingSurface == VK_NULL_HANDLE) {
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }
#endif

#ifdef VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME
    if (VulkanInstance::IsExtensionAvailable(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME)) {
        extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }
#endif

    std::sort(extensions.begin(), extensions.end(), [](const char* a, const char* b) {
        return std::strcmp(a, b) < 0;
    });
    extensions.erase(std::unique(extensions.begin(), extensions.end(), [](const char* a, const char* b) {
        return std::strcmp(a, b) == 0;
    }), extensions.end());
    return extensions;
}

void VulkanBackend::Initialize(const VulkanBackendConfig& config) {
    if (initialized_) {
        throw std::runtime_error("VulkanBackend is already initialized");
    }

    config_ = config;
    uiState_.mode = config_.uiTestMode;

    const auto instanceExtensions = BuildRequiredInstanceExtensions(config_);
    instance_.Create(config_.applicationName, config_.enableValidation, instanceExtensions);
    CreateSurface(config_);
    device_.Create(instance_.Get(), surface_.Get(), instance_.ValidationEnabled());
#ifndef NDEBUG
    if (!debugDeviceLogged_) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device_.Physical(), &properties);
        std::cerr << "Vulkan GPU: " << properties.deviceName << '\n';
        debugDeviceLogged_ = true;
    }
#endif
    commandContext_.Create(device_.Logical(), device_.QueueFamilies().graphicsFamily.value());
    sceneRenderer2D_.Create(device_);
    presentRenderer2D_.Create(device_);
    fsrEasuRenderer2D_.Create(device_);
    fsrRcasRenderer2D_.Create(device_);
    if (config_.drawSmokeGeometry && !config_.smokeTexturePath.empty()) {
        const LoadedImage smokeImage = LoadRgbaImage(config_.smokeTexturePath);
        smokeTexture_.CreateRGBA8(device_,
                                  commandContext_.Pool(),
                                  device_.GraphicsQueue(),
                                  smokeImage.width,
                                  smokeImage.height,
                                  smokeImage.rgba.data(),
                                  VK_FILTER_LINEAR);
    } else if (config_.drawSmokeGeometry) {
        const std::array<std::uint32_t, 4> smokePixels = {
            0xff2020d6u,
            0xffeeeeeeu,
            0xffeeeeeeu,
            0xff2020d6u
        };
        smokeTexture_.CreateRGBA8(device_,
                                  commandContext_.Pool(),
                                  device_.GraphicsQueue(),
                                  2,
                                  2,
                                  smokePixels.data(),
                                  VK_FILTER_NEAREST);
    }
    if (config_.uiTestMode != VulkanUiTestMode::None || config_.drawGameAssetSmoke) {
        LoadUiResources();
    }
    CreateSwapchainResources();

    initialized_ = true;
}

void VulkanBackend::CreateSurface(const VulkanBackendConfig& config) {
    if (config.existingSurface != VK_NULL_HANDLE) {
        surface_.SetExisting(instance_.Get(), config.existingSurface, config.ownsExistingSurface);
        return;
    }

    surface_.CreateWin32(instance_.Get(), config.win32Hinstance, config.win32Hwnd);
}

void VulkanBackend::Shutdown() {
    if (!initialized_ && instance_.Get() == VK_NULL_HANDLE) {
        return;
    }

    WaitIdle();

    rendererScissorStack_.clear();
    rendererTargetTextureOwners_.clear();
    for (auto& [_, target] : rendererTargets_) {
        if (target) {
            target->target.Destroy(device_.Logical());
        }
    }
    rendererTargets_.clear();
    for (auto& [_, texture] : rendererTextures_) {
        if (texture) {
            texture->texture.Destroy(device_.Logical());
        }
    }
    rendererTextures_.clear();
    for (auto& [_, font] : rendererFonts_) {
        if (font) {
            font->font.Destroy(device_.Logical());
        }
    }
    rendererFonts_.clear();
    unsupportedWarnings_.clear();

    DestroySwapchainResources();
    DestroyUiResources();
    smokeTexture_.Destroy(device_.Logical());
    fsrRcasRenderer2D_.Destroy();
    fsrEasuRenderer2D_.Destroy();
    presentRenderer2D_.Destroy();
    sceneRenderer2D_.Destroy();
    commandContext_.Destroy();
    device_.Destroy();
    surface_.Destroy();
    instance_.Destroy();

    initialized_ = false;
    frameOpen_ = false;
    clearRecorded_ = false;
    logicalPassOpen_ = false;
    logicalPassRecorded_ = false;
    presentPassRecorded_ = false;
    currentFrame_ = 0;
    currentImageIndex_ = 0;
    nextRendererTextureId_ = 1;
    nextRendererFontId_ = 1;
    nextRendererTargetId_ = 1;
    debugDeviceLogged_ = false;
    debugSwapchainLogged_ = false;
}

void VulkanBackend::WaitIdle() const {
    if (device_.Logical() != VK_NULL_HANDLE) {
        CheckVk(vkDeviceWaitIdle(device_.Logical()), "waiting for Vulkan device idle");
    }
}

void VulkanBackend::CreateSwapchainResources() {
    displayHdrState_ = hdrProbe_.QueryDisplayState(config_.win32Hwnd);
    const VulkanHdrMode swapchainHdrMode =
        config_.hdrMode == VulkanHdrMode::Auto && !displayHdrState_.hdrEnabled
            ? VulkanHdrMode::Off
            : config_.hdrMode;
    swapchain_.Create(device_,
                      surface_.Get(),
                      config_.windowWidth,
                      config_.windowHeight,
                      swapchainHdrMode,
                      config_.vsync);
    hdrMetadataDirty_ = true;
    ApplyHdrMetadata();
    constexpr VkFormat sceneFormat = VK_FORMAT_R8G8B8A8_UNORM;
    activeSampleCount_ = ChooseSampleCount(device_.Physical(), config_.msaaSamples);
    sceneRenderPass_.Create(device_.Logical(),
                            sceneFormat,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            activeSampleCount_);
    swapchainRenderPass_.Create(device_.Logical(), swapchain_.ImageFormat(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    const VkExtent2D logicalTargetExtent = SelectedRenderExtent(config_);
    logicalTarget_.Create(device_,
                          sceneRenderPass_.Get(),
                          logicalTargetExtent.width,
                          logicalTargetExtent.height,
                           sceneFormat,
                           config_.outputLinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
                           activeSampleCount_);
    framebuffers_.Create(device_.Logical(), swapchainRenderPass_.Get(), swapchain_.ImageViews(), swapchain_.Extent());
    sync_.Create(device_.Logical(), static_cast<std::uint32_t>(swapchain_.Images().size()));
    scenePipeline_.CreateSolid2D(device_.Logical(),
                                 sceneRenderPass_.Get(),
                                 logicalTarget_.Extent(),
                                 config_.shaderDirectory,
                                 "textured2d.frag.spv",
                                 activeSampleCount_);
    fsrEasuPipeline_.CreateSolid2D(device_.Logical(),
                                   sceneRenderPass_.Get(),
                                   logicalTarget_.Extent(),
                                   config_.shaderDirectory,
                                   "fsr1_easu.frag.spv",
                                   activeSampleCount_);
    fsrRcasPipeline_.CreateSolid2D(device_.Logical(),
                                   sceneRenderPass_.Get(),
                                   logicalTarget_.Extent(),
                                   config_.shaderDirectory,
                                   "fsr1_rcas.frag.spv",
                                   activeSampleCount_);
    presentPipeline_.CreateSolid2D(device_.Logical(),
                                   swapchainRenderPass_.Get(),
                                   swapchain_.Extent(),
                                   config_.shaderDirectory,
                                   "present2d.frag.spv",
                                   VK_SAMPLE_COUNT_1_BIT);
    if (smokeTexture_.ImageView() != VK_NULL_HANDLE) {
        smokeTextureDescriptor_ = scenePipeline_.CreateTextureDescriptorSet(smokeTexture_.ImageView(), smokeTexture_.Sampler());
    }
    CreateUiDescriptors();
    logicalTargetDescriptor_ = presentPipeline_.CreateTextureDescriptorSet(logicalTarget_.ImageView(), logicalTarget_.Sampler());
#ifndef NDEBUG
    if (!debugSwapchainLogged_) {
        const VkRect2D rect = LetterboxRect();
        std::cerr << "Vulkan selected-resolution target: " << logicalTarget_.Extent().width << "x" << logicalTarget_.Extent().height
                  << ", final presentation rect: " << rect.offset.x << "," << rect.offset.y
                  << " " << rect.extent.width << "x" << rect.extent.height
                  << ", swapchain format=" << swapchain_.ImageFormat()
                  << ", colorSpace=" << swapchain_.ColorSpace() << '\n';
        debugSwapchainLogged_ = true;
    }
#endif
}

void VulkanBackend::ApplyHdrMetadata() {
    hdrMetadataDirty_ = false;
    if (!swapchain_.HdrActive() || !device_.HdrMetadataEnabled()) {
        return;
    }
    const auto setMetadata = reinterpret_cast<PFN_vkSetHdrMetadataEXT>(
        vkGetDeviceProcAddr(device_.Logical(), "vkSetHdrMetadataEXT")
    );
    if (!setMetadata) {
        return;
    }

    VkHdrMetadataEXT metadata{};
    metadata.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
    if (swapchain_.ColorSpace() == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
        metadata.displayPrimaryRed = {0.708f, 0.292f};
        metadata.displayPrimaryGreen = {0.170f, 0.797f};
        metadata.displayPrimaryBlue = {0.131f, 0.046f};
    } else {
        metadata.displayPrimaryRed = {0.640f, 0.330f};
        metadata.displayPrimaryGreen = {0.300f, 0.600f};
        metadata.displayPrimaryBlue = {0.150f, 0.060f};
    }
    metadata.whitePoint = {0.3127f, 0.3290f};
    const float paperWhiteNits = EffectiveHdrPaperWhiteNits();
    const float peakNits = EffectiveHdrPeakNits();
    metadata.maxLuminance = peakNits;
    metadata.minLuminance = 0.001f;
    metadata.maxContentLightLevel = peakNits;
    metadata.maxFrameAverageLightLevel = std::min(
        peakNits,
        std::max(paperWhiteNits, peakNits * 0.4f)
    );
    const VkSwapchainKHR swapchain = swapchain_.Get();
    setMetadata(device_.Logical(), 1, &swapchain, &metadata);
}

float VulkanBackend::EffectiveHdrPaperWhiteNits() const noexcept {
    if (config_.hdrMode == VulkanHdrMode::Auto && displayHdrState_.hdrEnabled) {
        return std::clamp(displayHdrState_.sdrWhiteLevelNits, 80.0f, 500.0f);
    }
    return std::clamp(postProcess_.hdrPaperWhiteNits, 80.0f, 500.0f);
}

float VulkanBackend::EffectiveHdrPeakNits() const noexcept {
    return std::clamp(
        std::max(postProcess_.hdrPeakNits, EffectiveHdrPaperWhiteNits()),
        400.0f,
        4000.0f
    );
}

void VulkanBackend::DestroySwapchainResources() {
    smokeTextureDescriptor_ = VK_NULL_HANDLE;
    logicalTargetDescriptor_ = VK_NULL_HANDLE;
    uiFontDescriptor_ = VK_NULL_HANDLE;
    uiBgPreload_.descriptor = VK_NULL_HANDLE;
    uiBgPreload2_.descriptor = VK_NULL_HANDLE;
    uiBg_.descriptor = VK_NULL_HANDLE;
    uiTv_.descriptor = VK_NULL_HANDLE;
    for (auto& [_, texture] : rendererTextures_) {
        if (texture) {
            texture->descriptor = VK_NULL_HANDLE;
        }
    }
    for (auto& [_, font] : rendererFonts_) {
        if (font) {
            font->descriptor = VK_NULL_HANDLE;
        }
    }
    for (auto& [_, target] : rendererTargets_) {
        if (target) {
            target->descriptor = VK_NULL_HANDLE;
        }
    }
    fsrIntermediateTarget_.Destroy(device_.Logical());
    fsrRcasPipeline_.Destroy();
    fsrEasuPipeline_.Destroy();
    presentPipeline_.Destroy();
    scenePipeline_.Destroy();
    logicalTarget_.Destroy(device_.Logical());
    sync_.Destroy();
    framebuffers_.Destroy();
    swapchainRenderPass_.Destroy();
    sceneRenderPass_.Destroy();
    swapchain_.Destroy();
}

void VulkanBackend::Resize(std::uint32_t width, std::uint32_t height) {
    if (!initialized_) {
        throw std::runtime_error("Resize called before VulkanBackend initialization");
    }
    if (frameOpen_) {
        throw std::runtime_error("Resize called while a Vulkan frame is open");
    }
    if (width == 0 || height == 0) {
        return;
    }
    if (config_.windowWidth == width && config_.windowHeight == height) {
        return;
    }

    CheckVk(vkDeviceWaitIdle(device_.Logical()), "waiting for Vulkan device before swapchain resize");
    config_.windowWidth = width;
    config_.windowHeight = height;
    DestroySwapchainResources();
    CreateSwapchainResources();
    currentFrame_ = 0;
    currentImageIndex_ = 0;
    clearRecorded_ = false;
}

void VulkanBackend::SetRenderResolution(std::uint32_t width, std::uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    if (config_.renderWidth == width && config_.renderHeight == height) {
        return;
    }
    if (frameOpen_ || rendererTargetPassOpen_) {
        throw std::runtime_error("SetRenderResolution called while a Vulkan render pass is open");
    }
    config_.renderWidth = width;
    config_.renderHeight = height;
    if (initialized_) {
        RecreateSwapchain();
    }
}

void VulkanBackend::RecreateSwapchain() {
    if (!initialized_) {
        return;
    }
    if (frameOpen_) {
        throw std::runtime_error("RecreateSwapchain called while a Vulkan frame is open");
    }
    CheckVk(vkDeviceWaitIdle(device_.Logical()), "waiting for Vulkan device before swapchain recreation");
    DestroySwapchainResources();
    CreateSwapchainResources();
    currentFrame_ = 0;
    currentImageIndex_ = 0;
    clearRecorded_ = false;
}

void VulkanBackend::SetUiState(const VulkanUiRuntimeState& state) {
    uiState_ = state;
}

void VulkanBackend::SetFinalPresentationRect(wb::render::Rect rect) {
    VkRect2D value{};
    value.offset.x = static_cast<std::int32_t>(std::lround(rect.x));
    value.offset.y = static_cast<std::int32_t>(std::lround(rect.y));
    value.extent.width = static_cast<std::uint32_t>(std::max(1.0f, std::round(rect.width)));
    value.extent.height = static_cast<std::uint32_t>(std::max(1.0f, std::round(rect.height)));
    finalPresentationRect_ = value;
}

void VulkanBackend::SetPresentationOptions(bool vsync, bool outputLinearFilter) {
    if (config_.vsync == vsync && config_.outputLinearFilter == outputLinearFilter) {
        return;
    }
    if (!initialized_) {
        config_.vsync = vsync;
        config_.outputLinearFilter = outputLinearFilter;
        return;
    }
    if (frameOpen_) {
        throw std::runtime_error("SetPresentationOptions called while a Vulkan frame is open");
    }

    CheckVk(vkDeviceWaitIdle(device_.Logical()), "waiting for Vulkan device before presentation option change");
    config_.vsync = vsync;
    config_.outputLinearFilter = outputLinearFilter;
    DestroySwapchainResources();
    CreateSwapchainResources();
    currentFrame_ = 0;
    currentImageIndex_ = 0;
    clearRecorded_ = false;
}

void VulkanBackend::SetVsync(bool enabled) {
    SetPresentationOptions(enabled, config_.outputLinearFilter);
}

void VulkanBackend::SetOutputLinearFilter(bool enabled) {
    SetPresentationOptions(config_.vsync, enabled);
}

void VulkanBackend::SetHdrMode(VulkanHdrMode mode) {
    if (config_.hdrMode == mode) {
        return;
    }
    if (!initialized_) {
        config_.hdrMode = mode;
        return;
    }
    if (frameOpen_) {
        throw std::runtime_error("SetHdrMode called while a Vulkan frame is open");
    }
    config_.hdrMode = mode;
    RecreateSwapchain();
}

void VulkanBackend::SetPostProcessSettings(const wb::render::PostProcessSettings& settings) {
    if (postProcess_.hdrPaperWhiteNits != settings.hdrPaperWhiteNits ||
        postProcess_.hdrPeakNits != settings.hdrPeakNits) {
        hdrMetadataDirty_ = true;
    }
    postProcess_ = settings;
}

void VulkanBackend::SetMsaaSamples(std::uint32_t samples) {
    const std::uint32_t requested = samples >= 16 ? 16 : samples >= 8 ? 8 : samples >= 4 ? 4 : samples >= 2 ? 2 : 1;
    config_.msaaSamples = requested;
    if (!initialized_) {
        return;
    }
    const VkSampleCountFlagBits desired = ChooseSampleCount(device_.Physical(), requested);
    if (activeSampleCount_ == desired) {
        return;
    }
    if (frameOpen_ || rendererTargetPassOpen_) {
        throw std::runtime_error("SetMsaaSamples called while a Vulkan render pass is open");
    }
    if (!rendererTargets_.empty()) {
        WarnUnsupportedOnce("renderer.msaa_live_target",
                            "Warning: Vulkan MSAA cannot change while gameplay render targets are alive; the new value will apply next launch.\n");
        return;
    }
    RecreateSwapchain();
}

std::uint32_t VulkanBackend::ActiveMsaaSamples() const noexcept {
    return SampleCountValue(activeSampleCount_);
}

VkRect2D VulkanBackend::FinalPresentationRect() const noexcept {
    return LetterboxRect();
}

bool VulkanBackend::BeginFrame() {
    if (!initialized_) {
        throw std::runtime_error("BeginFrame called before VulkanBackend initialization");
    }
    if (frameOpen_) {
        throw std::runtime_error("BeginFrame called while a Vulkan frame is already open");
    }
    if (hdrMetadataDirty_) {
        ApplyHdrMetadata();
    }

    VkFence fence = sync_.InFlight(currentFrame_);
    CheckVk(vkWaitForFences(device_.Logical(), 1, &fence, VK_TRUE, UINT64_MAX),
            "waiting for Vulkan in-flight fence");

    VkResult acquireResult = vkAcquireNextImageKHR(
        device_.Logical(),
        swapchain_.Get(),
        UINT64_MAX,
        sync_.ImageAvailable(currentFrame_),
        VK_NULL_HANDLE,
        &currentImageIndex_);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(MakeVkErrorMessage(acquireResult, "acquiring Vulkan swapchain image"));
    }

    CheckVk(vkResetFences(device_.Logical(), 1, &fence), "resetting Vulkan in-flight fence");

    commandContext_.BeginFrame(currentFrame_);
    frameOpen_ = true;
    clearRecorded_ = false;
    logicalPassOpen_ = false;
    logicalPassRecorded_ = false;
    presentPassRecorded_ = false;
    rendererScissorStack_.clear();
    return true;
}

void VulkanBackend::ClearFrame() {
    if (!frameOpen_) {
        throw std::runtime_error("ClearFrame called before BeginFrame");
    }
    if (clearRecorded_) {
        return;
    }

    RecordFrame(commandContext_.Buffer(currentFrame_), currentImageIndex_);
    clearRecorded_ = true;
    logicalPassRecorded_ = true;
    presentPassRecorded_ = true;
}

void VulkanBackend::BeginLogicalRender() {
    if (!frameOpen_) {
        throw std::runtime_error("BeginLogicalRender called before BeginFrame");
    }
    if (clearRecorded_ || logicalPassRecorded_) {
        throw std::runtime_error("BeginLogicalRender called after this frame already recorded a logical pass");
    }
    if (logicalPassOpen_) {
        throw std::runtime_error("BeginLogicalRender called while a logical pass is already open");
    }

    VkClearValue clearColor{};
    clearColor.color = {{config_.clearColor[0], config_.clearColor[1], config_.clearColor[2], config_.clearColor[3]}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = sceneRenderPass_.Get();
    renderPassInfo.framebuffer = logicalTarget_.Framebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = logicalTarget_.Extent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    VkCommandBuffer commandBuffer = commandContext_.Buffer(currentFrame_);
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    sceneRenderer2D_.BeginFrame(commandBuffer,
                                scenePipeline_.Get(),
                                scenePipeline_.Layout(),
                                scenePipeline_.Textured(),
                                scenePipeline_.TexturedLayout(),
                                logicalTarget_.Extent(),
                                currentFrame_,
                                {960, 540});
    logicalPassOpen_ = true;
}

void VulkanBackend::EndLogicalRender() {
    if (!logicalPassOpen_) {
        throw std::runtime_error("EndLogicalRender called before BeginLogicalRender");
    }
    ClearRendererScissor();
    sceneRenderer2D_.EndFrame();
    vkCmdEndRenderPass(commandContext_.Buffer(currentFrame_));
    logicalPassOpen_ = false;
    logicalPassRecorded_ = true;
}

void VulkanBackend::Present() {
    if (!frameOpen_) {
        throw std::runtime_error("Present called before BeginFrame");
    }
    if (logicalPassOpen_) {
        throw std::runtime_error("Present called before EndLogicalRender");
    }
    if (!logicalPassRecorded_) {
        throw std::runtime_error("Present called before a logical pass was recorded");
    }
    if (presentPassRecorded_) {
        return;
    }
    RecordPresentPass(commandContext_.Buffer(currentFrame_), currentImageIndex_);
    presentPassRecorded_ = true;
}

VkRect2D VulkanBackend::LetterboxRect() const noexcept {
    if (finalPresentationRect_.has_value()) {
        return *finalPresentationRect_;
    }
    const VkExtent2D extent = swapchain_.Extent();
    constexpr float logicalWidth = 960.0f;
    constexpr float logicalHeight = 540.0f;
    const float scale = std::min(static_cast<float>(extent.width) / logicalWidth,
                                 static_cast<float>(extent.height) / logicalHeight);
    const std::uint32_t width = static_cast<std::uint32_t>(logicalWidth * scale);
    const std::uint32_t height = static_cast<std::uint32_t>(logicalHeight * scale);
    VkRect2D rect{};
    rect.offset.x = static_cast<std::int32_t>((extent.width - width) / 2);
    rect.offset.y = static_cast<std::int32_t>((extent.height - height) / 2);
    rect.extent.width = width;
    rect.extent.height = height;
    return rect;
}

void VulkanBackend::RecordFrame(VkCommandBuffer commandBuffer, std::uint32_t imageIndex) {
    RecordLogicalScenePass(commandBuffer);
    RecordPresentPass(commandBuffer, imageIndex);
}

void VulkanBackend::RecordLogicalScenePass(VkCommandBuffer commandBuffer) {
    VkClearValue clearColor{};
    clearColor.color = {{config_.clearColor[0], config_.clearColor[1], config_.clearColor[2], config_.clearColor[3]}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = sceneRenderPass_.Get();
    renderPassInfo.framebuffer = logicalTarget_.Framebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = logicalTarget_.Extent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    sceneRenderer2D_.BeginFrame(commandBuffer,
                                scenePipeline_.Get(),
                                scenePipeline_.Layout(),
                                scenePipeline_.Textured(),
                                scenePipeline_.TexturedLayout(),
                                logicalTarget_.Extent(),
                                currentFrame_,
                                {960, 540});
    if (uiState_.mode == VulkanUiTestMode::Menu) {
        DrawMenuTestScene();
    } else if (uiState_.mode == VulkanUiTestMode::Settings) {
        DrawSettingsTestScene();
    } else if (uiState_.mode == VulkanUiTestMode::SharedGameBoundary) {
        DrawSharedGameBoundaryScene();
    } else if (config_.drawSmokeGeometry) {
        DrawSmokeScene();
    }
    sceneRenderer2D_.EndFrame();
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanBackend::RecordPresentPass(VkCommandBuffer commandBuffer, std::uint32_t imageIndex) {
    const auto& fbs = framebuffers_.Get();
    if (imageIndex >= fbs.size()) {
        throw std::runtime_error("Swapchain image index is outside framebuffer range");
    }

    VkClearValue clearColor{};
    clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapchainRenderPass_.Get();
    renderPassInfo.framebuffer = fbs[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain_.Extent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    const VkRect2D letterbox = LetterboxRect();
    presentRenderer2D_.BeginFrame(commandBuffer,
                                  presentPipeline_.Get(),
                                  presentPipeline_.Layout(),
                                  presentPipeline_.Textured(),
                                  presentPipeline_.TexturedLayout(),
                                  swapchain_.Extent(),
                                  currentFrame_);
    VulkanRenderer2D::PostProcessSettings presentSettings;
    presentSettings.textureWidth = static_cast<float>(logicalTarget_.Extent().width);
    presentSettings.textureHeight = static_cast<float>(logicalTarget_.Extent().height);
    presentSettings.time = postProcess_.time;
    presentSettings.useFxaa = postProcess_.fxaa ? 1.0f : 0.0f;
    presentSettings.crtStrength = postProcess_.crtStrength;
    presentSettings.noiseAmount = postProcess_.noiseAmount;
    presentSettings.brightness = postProcess_.brightness;
    presentSettings.contrast = postProcess_.contrast;
    presentSettings.sharpness = postProcess_.sharpness;
    presentSettings.gamma = postProcess_.gamma;
    presentSettings.blackLevel = postProcess_.blackLevel;
    presentSettings.whiteLevel = postProcess_.whiteLevel;
    presentSettings.screenBorder = postProcess_.screenBorder;
    presentSettings.scanlines = postProcess_.scanlines;
    presentSettings.crtCurve = postProcess_.crtCurve;
    presentSettings.hdrEncoding = OutputEncodingFor(
        swapchain_.ImageFormat(),
        swapchain_.ColorSpace(),
        swapchain_.HdrActive()
    );
    presentSettings.hdrPaperWhiteNits = EffectiveHdrPaperWhiteNits();
    presentSettings.hdrMaxNits = EffectiveHdrPeakNits();
    presentSettings.hdrHighlightStrength = std::clamp(postProcess_.hdrHighlightStrength, 0.0f, 1.0f);
    presentRenderer2D_.SetPostProcessSettings(presentSettings);
    presentRenderer2D_.DrawTexturedRect(static_cast<float>(letterbox.offset.x),
                                        static_cast<float>(letterbox.offset.y),
                                        static_cast<float>(letterbox.extent.width),
                                        static_cast<float>(letterbox.extent.height),
                                        logicalTargetDescriptor_);
    presentRenderer2D_.EndFrame();
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanBackend::LoadUiResources() {
    if (config_.assetRoot.empty()) {
        throw std::runtime_error("Vulkan UI test requires an asset root");
    }

    LoadUiLocalization();

    auto loadTexture = [&](UiTexture& target, const char* relativePath) {
        const LoadedImage image = LoadRgbaImage(TexturePath(config_.assetRoot, relativePath));
        target.texture.CreateRGBA8(device_,
                                   commandContext_.Pool(),
                                   device_.GraphicsQueue(),
                                   image.width,
                                   image.height,
                                   image.rgba.data(),
                                   VK_FILTER_LINEAR);
#ifndef NDEBUG
        std::cerr << "Vulkan texture loaded: " << relativePath << " (" << image.width << "x" << image.height << ")\n";
#endif
    };

    if (config_.uiTestMode != VulkanUiTestMode::None) {
        loadTexture(uiBgPreload_, "sprites/bg_preload.png");
        loadTexture(uiBgPreload2_, "sprites/bg_preload_2.png");
        loadTexture(uiBg_, "sprites/bg.png");
        loadTexture(uiTv_, "sprites/tv.png");
    }

    std::filesystem::path fontPath = config_.fontPath;
    if (fontPath.empty()) {
        fontPath = "C:/Windows/Fonts/arial.ttf";
    }
    uiFont_.Create(device_, commandContext_.Pool(), device_.GraphicsQueue(), fontPath, 96);
#ifndef NDEBUG
    std::cerr << "Vulkan font atlas: " << uiFont_.AtlasWidth() << "x" << uiFont_.AtlasHeight()
              << ", image loader: WIC, font path: " << fontPath.u8string() << '\n';
#endif
}

void VulkanBackend::DestroyUiResources() {
    uiFont_.Destroy(device_.Logical());
    uiBgPreload_.texture.Destroy(device_.Logical());
    uiBgPreload2_.texture.Destroy(device_.Logical());
    uiBg_.texture.Destroy(device_.Logical());
    uiTv_.texture.Destroy(device_.Logical());
    uiBgPreload_.descriptor = VK_NULL_HANDLE;
    uiBgPreload2_.descriptor = VK_NULL_HANDLE;
    uiBg_.descriptor = VK_NULL_HANDLE;
    uiTv_.descriptor = VK_NULL_HANDLE;
    uiFontDescriptor_ = VK_NULL_HANDLE;
    uiStrings_.clear();
}

void VulkanBackend::CreateUiDescriptors() {
    auto createDescriptor = [&](UiTexture& texture) {
        if (texture.texture.ImageView() != VK_NULL_HANDLE) {
            texture.descriptor = scenePipeline_.CreateTextureDescriptorSet(texture.texture.ImageView(),
                                                                           texture.texture.Sampler());
        }
    };

    createDescriptor(uiBgPreload_);
    createDescriptor(uiBgPreload2_);
    createDescriptor(uiBg_);
    createDescriptor(uiTv_);
    if (uiFont_.Ready()) {
        uiFontDescriptor_ = scenePipeline_.CreateTextureDescriptorSet(uiFont_.ImageView(), uiFont_.Sampler());
    }
    for (auto& [_, texture] : rendererTextures_) {
        if (texture && texture->texture.ImageView() != VK_NULL_HANDLE) {
            texture->descriptor = scenePipeline_.CreateTextureDescriptorSet(texture->texture.ImageView(),
                                                                            texture->texture.Sampler());
        }
    }
    for (auto& [_, font] : rendererFonts_) {
        if (font && font->font.Ready()) {
            font->descriptor = scenePipeline_.CreateTextureDescriptorSet(font->font.ImageView(), font->font.Sampler());
        }
    }
    for (auto& [_, target] : rendererTargets_) {
        if (target && target->target.ImageView() != VK_NULL_HANDLE) {
            target->descriptor = scenePipeline_.CreateTextureDescriptorSet(target->target.ImageView(),
                                                                          target->target.Sampler());
        }
    }
}

void VulkanBackend::LoadUiLocalization() {
    uiStrings_ = {
        {"menu.title.line1", "WE BECOME WHAT"},
        {"menu.title.line2", "WE BEHOLD"},
        {"menu.subtitle", "a game about news cycles, vicious cycles, infinite cycles"},
        {"menu.play", "PLAY"},
        {"menu.settings", "SETTINGS"},
        {"menu.playing_time", "playing time: 5 minutes"},
        {"menu.warning", "warning: the following program\ncontains scenes of snobbery,\nrudeness & mass murder.\nviewer discretion is advised."},
        {"settings.title", "SETTINGS"},
        {"settings.tab.audio", "AUDIO"},
        {"settings.tab.display", "DISPLAY"},
        {"settings.tab.visuals", "VISUALS"},
        {"settings.tab.monitor", "MONITOR"},
        {"settings.category", "Category"},
        {"settings.language", "Language"},
        {"settings.fullscreen", "Fullscreen"},
        {"settings.window_size", "Window Size"},
        {"settings.vsync", "VSync"},
        {"settings.output_smoothing", "Output Smoothing"},
        {"settings.msaa", "MSAA"},
        {"settings.master_volume", "Master Volume"},
        {"settings.music_volume", "Music Volume"},
        {"settings.sfx_volume", "SFX Volume"},
        {"settings.display_mode", "Display Mode"},
        {"settings.fxaa", "FXAA"},
        {"settings.frame_limit", "Frame Limit"},
        {"settings.reset", "Reset Settings"},
        {"settings.back", "Back"},
        {"settings.value.on", "On"},
        {"settings.value.off", "Off"},
        {"settings.value.nearest", "Nearest"},
        {"settings.value.linear", "Linear"},
        {"settings.value.esc", "Esc"},
        {"chyron.crazy_square_attacks", "CRAZED SQUARE ATTACKS"},
        {"chyron.nice_hat", "OOH NICE HAT"},
        {"vulkan.shared_path.title", "VULKAN SHARED PATH"},
        {"vulkan.shared_path.status", "Shared Game and SceneManager are active."},
        {"vulkan.shared_path.blocker", "GameplayScene now renders through Vulkan with\nWin32 input, miniaudio, and full-quality photo capture."},
        {"vulkan.shared_path.next", "The automated shared-game test verifies the first\nreal camera and TV gameplay cycle."},
        {"vulkan.shared_path.back", "Press Esc or Backspace to return to the menu."}
    };

    if (config_.localizationPath.empty() || !std::filesystem::exists(config_.localizationPath)) {
        return;
    }

    std::ifstream input(config_.localizationPath, std::ios::binary);
    if (!input) {
        return;
    }

    try {
        nlohmann::json json;
        input >> json;
        if (!json.contains("strings") || !json.at("strings").is_object()) {
            return;
        }
        for (auto& [key, value] : uiStrings_) {
            const auto it = json.at("strings").find(key);
            if (it != json.at("strings").end() && it->is_string()) {
                value = it->get<std::string>();
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Warning: Vulkan UI localization load failed: " << ex.what() << '\n';
    }
}

const std::string& VulkanBackend::UiString(std::string_view key) const {
    static const std::string kMissing = "?";
    const auto it = uiStrings_.find(std::string(key));
    return it != uiStrings_.end() ? it->second : kMissing;
}

void VulkanBackend::DrawRectOutline(float x, float y, float width, float height, float thickness, VulkanRenderer2D::Color color) {
    sceneRenderer2D_.DrawRect(x, y, width, thickness, color);
    sceneRenderer2D_.DrawRect(x, y + height - thickness, width, thickness, color);
    sceneRenderer2D_.DrawRect(x, y, thickness, height, color);
    sceneRenderer2D_.DrawRect(x + width - thickness, y, thickness, height, color);
}

void VulkanBackend::DrawText(std::string_view key, float x, float y, float size, VulkanRenderer2D::Color color) {
    DrawTextRaw(UiString(key), x, y, size, color);
}

void VulkanBackend::DrawTextRaw(std::string_view text, float x, float y, float size, VulkanRenderer2D::Color color) {
    uiFont_.DrawText(sceneRenderer2D_, uiFontDescriptor_, text, x, y, size, color);
}

void VulkanBackend::DrawTextCentered(std::string_view text, float centerX, float centerY, float size, VulkanRenderer2D::Color color) {
    const VulkanFont::TextExtent measured = uiFont_.MeasureText(text, size);
    DrawTextRaw(text, centerX - measured.width * 0.5f, centerY - measured.height * 0.5f, size, color);
}

void VulkanBackend::DrawTextCenteredFit(std::string_view text,
                                        float centerX,
                                        float centerY,
                                        float size,
                                        float maxWidth,
                                        VulkanRenderer2D::Color color) {
    float fitted = size;
    while (fitted > 6.0f && uiFont_.MeasureText(text, fitted).width > maxWidth) {
        fitted -= 0.5f;
    }
    DrawTextCentered(text, centerX, centerY, fitted, color);
}

void VulkanBackend::DrawSmokeScene() {
    constexpr float w = 960.0f;
    constexpr float h = 540.0f;
    if (config_.drawGameAssetSmoke && smokeTextureDescriptor_ != VK_NULL_HANDLE) {
        constexpr float screenX = 96.0f;
        constexpr float screenY = 72.0f;
        constexpr float screenW = 768.0f;
        constexpr float screenH = 390.0f;
        const VulkanRenderer2D::AtlasFrame circleFrame{
            {0.0f, 0.0f, 240.0f, 280.0f},
            {0.0f, 0.0f, 240.0f, 280.0f},
            240.0f,
            280.0f,
            false
        };
        const VulkanRenderer2D::AtlasFrame squareFrame{
            {240.0f, 0.0f, 240.0f, 280.0f},
            {0.0f, 0.0f, 240.0f, 280.0f},
            240.0f,
            280.0f,
            false
        };

        sceneRenderer2D_.DrawRect(62.0f, 44.0f, 836.0f, 458.0f, Rgba(0.03f, 0.03f, 0.035f));
        sceneRenderer2D_.DrawRect(screenX - 10.0f, screenY - 10.0f, screenW + 20.0f, screenH + 20.0f, Rgba(0.24f, 0.24f, 0.25f));
        sceneRenderer2D_.DrawRect(screenX, screenY, screenW, screenH, Rgba(0.78f, 0.78f, 0.76f));
        sceneRenderer2D_.SetScissor(screenX, screenY, screenW, screenH);
        sceneRenderer2D_.DrawRect(screenX, screenY, screenW, screenH, Rgba(0.72f, 0.72f, 0.70f));
        sceneRenderer2D_.DrawRect(screenX + 24.0f, screenY + 285.0f, screenW - 48.0f, 18.0f, Rgba(0.77f, 0.11f, 0.10f, 0.82f));
        sceneRenderer2D_.DrawAtlasFrame(280.0f, 390.0f, 0.39f, 0.39f, 0.5f, 1.0f, smokeTextureDescriptor_, smokeTexture_.Width(), smokeTexture_.Height(), circleFrame);
        sceneRenderer2D_.DrawAtlasFrame(390.0f, 390.0f, 0.39f, 0.39f, 0.5f, 1.0f, smokeTextureDescriptor_, smokeTexture_.Width(), smokeTexture_.Height(), squareFrame);
        sceneRenderer2D_.DrawTexturedSourceRect(505.0f, 275.0f, 96.0f, 112.0f, smokeTextureDescriptor_, smokeTexture_.Width(), smokeTexture_.Height(), {240.0f, 0.0f, 240.0f, 280.0f}, Rgba(1.0f, 1.0f, 1.0f, 0.78f));
        sceneRenderer2D_.DrawAtlasFrame(700.0f, 410.0f, -0.32f, 0.32f, 0.5f, 1.0f, smokeTextureDescriptor_, smokeTexture_.Width(), smokeTexture_.Height(), circleFrame, Rgba(1.0f, 1.0f, 1.0f, 0.88f));
        DrawText("chyron.crazy_square_attacks", screenX + 44.0f, screenY + 315.0f, 25.0f, Rgba(1.0f, 1.0f, 1.0f));
        sceneRenderer2D_.ClearScissor();
        sceneRenderer2D_.DrawRect(screenX - 10.0f, screenY - 10.0f, screenW + 20.0f, 12.0f, Rgba(0.14f, 0.14f, 0.145f));
        sceneRenderer2D_.DrawRect(screenX - 10.0f, screenY + screenH - 2.0f, screenW + 20.0f, 12.0f, Rgba(0.14f, 0.14f, 0.145f));
        sceneRenderer2D_.DrawRect(screenX - 10.0f, screenY - 10.0f, 12.0f, screenH + 20.0f, Rgba(0.14f, 0.14f, 0.145f));
        sceneRenderer2D_.DrawRect(screenX + screenW - 2.0f, screenY - 10.0f, 12.0f, screenH + 20.0f, Rgba(0.14f, 0.14f, 0.145f));
    } else {
        sceneRenderer2D_.DrawRect(w * 0.10f, h * 0.12f, w * 0.80f, h * 0.76f, Rgba(0.07f, 0.07f, 0.08f, 0.92f));
        sceneRenderer2D_.DrawRect(w * 0.12f, h * 0.16f, w * 0.76f, h * 0.12f, Rgba(0.82f, 0.12f, 0.13f));
        sceneRenderer2D_.DrawRect(w * 0.16f, h * 0.35f, w * 0.26f, h * 0.38f, Rgba(0.88f, 0.88f, 0.86f));
        sceneRenderer2D_.DrawRect(w * 0.46f, h * 0.35f, w * 0.16f, h * 0.38f, Rgba(0.62f, 0.62f, 0.62f));
        sceneRenderer2D_.DrawRect(w * 0.66f, h * 0.35f, w * 0.18f, h * 0.38f, Rgba(0.88f, 0.88f, 0.86f));
        sceneRenderer2D_.DrawTexturedRect(w * 0.43f, h * 0.42f, w * 0.20f, h * 0.20f, smokeTextureDescriptor_);
        sceneRenderer2D_.DrawCircle(w * 0.27f, h * 0.58f, 34.0f, Rgba(0.18f, 0.18f, 0.19f, 0.82f));
        sceneRenderer2D_.DrawTexturedSourceRectTransformed(w * 0.61f,
                                                           h * 0.55f,
                                                           84.0f,
                                                           84.0f,
                                                           42.0f,
                                                           42.0f,
                                                           0.32f,
                                                           smokeTextureDescriptor_,
                                                           smokeTexture_.Width(),
                                                           smokeTexture_.Height(),
                                                           {0.0f, 0.0f, static_cast<float>(smokeTexture_.Width()), static_cast<float>(smokeTexture_.Height())},
                                                           Rgba(1.0f, 1.0f, 1.0f, 0.72f));
        sceneRenderer2D_.DrawRect(w * 0.12f, h * 0.78f, w * 0.76f, h * 0.06f, Rgba(0.82f, 0.12f, 0.13f));
    }
}

void VulkanBackend::DrawMenuTestScene() {
    const VulkanMenuRuntimeState& menu = uiState_.menu;
    if (uiBgPreload_.descriptor != VK_NULL_HANDLE) {
        sceneRenderer2D_.DrawTexturedSourceRect(0.0f, 0.0f, 960.0f, 540.0f, uiBgPreload_.descriptor, uiBgPreload_.texture.Width(), uiBgPreload_.texture.Height(), {0.0f, 0.0f, static_cast<float>(uiBgPreload_.texture.Width()), static_cast<float>(uiBgPreload_.texture.Height())});
    }
    sceneRenderer2D_.DrawRect(40.0f, 34.0f, 610.0f, 172.0f, Rgba(0.675f, 0.675f, 0.675f));
    DrawText("menu.title.line1", 78.0f, 46.0f, 42.0f, Rgba(1.0f, 1.0f, 1.0f));
    DrawText("menu.title.line2", 78.0f, 88.0f, 67.0f, Rgba(1.0f, 1.0f, 1.0f));
    DrawText("menu.subtitle", 84.0f, 165.0f, 20.0f, Rgba(1.0f, 1.0f, 1.0f));

    const float playScale = menu.playHoverScale > 0.0f ? menu.playHoverScale : 1.0f;
    const float playX = 278.0f - 197.5f * playScale;
    const float playY = 250.0f - 42.5f * playScale;
    const float playW = 395.0f * playScale;
    const float playH = 85.0f * playScale;
    sceneRenderer2D_.DrawRect(playX, playY, playW, playH, menu.playHovered || menu.selected == 0 ? Rgba(0.96f, 0.14f, 0.16f) : Rgba(0.83f, 0.14f, 0.15f));
    DrawRectOutline(playX, playY, playW, playH, 4.0f, Rgba(0.0f, 0.0f, 0.0f));
    DrawTextCenteredFit(UiString("menu.play"), 278.0f, 250.0f, 45.0f * playScale, playW - 34.0f, Rgba(1.0f, 1.0f, 1.0f));

    if (uiBgPreload2_.descriptor != VK_NULL_HANDLE) {
        sceneRenderer2D_.DrawTexturedSourceRect(0.0f, 0.0f, 960.0f, 540.0f, uiBgPreload2_.descriptor, uiBgPreload2_.texture.Width(), uiBgPreload2_.texture.Height(), {0.0f, 0.0f, static_cast<float>(uiBgPreload2_.texture.Width()), static_cast<float>(uiBgPreload2_.texture.Height())});
    }

    const float settingsScale = menu.settingsHoverScale > 0.0f ? menu.settingsHoverScale : 1.0f;
    const VulkanRenderer2D::Color settingsColor = menu.settingsHovered || menu.selected == 1
        ? Rgba(0.93f, 0.93f, 0.93f, 0.95f)
        : Rgba(0.78f, 0.78f, 0.78f, 0.82f);
    DrawTextCenteredFit(UiString("menu.settings"), 778.0f, 424.0f, 14.0f * settingsScale, 82.0f, settingsColor);
    sceneRenderer2D_.DrawLine(740.0f, 434.0f, 816.0f, 434.0f, menu.settingsHovered || menu.selected == 1 ? 1.5f : 1.2f, Rgba(0.48f, 0.48f, 0.48f, 0.8f));
    sceneRenderer2D_.DrawRect(833.0f, 415.0f, 18.0f, 18.0f, Rgba(0.28f, 0.28f, 0.28f, 0.95f));
    DrawRectOutline(833.0f, 415.0f, 18.0f, 18.0f, 2.0f, settingsColor);

    DrawTextCentered(UiString("menu.playing_time"), 278.0f, 378.0f, 29.0f, Rgba(1.0f, 1.0f, 1.0f));
    DrawTextCentered(UiString("menu.warning"), 278.0f, 423.0f, 22.0f, Rgba(0.40f, 0.40f, 0.40f));
}

void VulkanBackend::DrawSettingsTestScene() {
    if (uiBg_.descriptor != VK_NULL_HANDLE) {
        sceneRenderer2D_.DrawTexturedSourceRect(-100.0f, -100.0f, static_cast<float>(uiBg_.texture.Width()), static_cast<float>(uiBg_.texture.Height()), uiBg_.descriptor, uiBg_.texture.Width(), uiBg_.texture.Height(), {0.0f, 0.0f, static_cast<float>(uiBg_.texture.Width()), static_cast<float>(uiBg_.texture.Height())});
    }

    constexpr float tvScale = 1.6f;
    constexpr float tvX = 480.0f;
    constexpr float tvY = 570.0f;
    const float tvW = static_cast<float>(uiTv_.texture.Width()) * tvScale;
    const float tvH = static_cast<float>(uiTv_.texture.Height()) * tvScale;
    const float tvLeft = tvX - tvW * 0.5f;
    const float tvTop = tvY - tvH;
    if (uiTv_.descriptor != VK_NULL_HANDLE) {
        sceneRenderer2D_.DrawTexturedSourceRect(tvLeft, tvTop, tvW, tvH, uiTv_.descriptor, uiTv_.texture.Width(), uiTv_.texture.Height(), {0.0f, 0.0f, static_cast<float>(uiTv_.texture.Width()), static_cast<float>(uiTv_.texture.Height())});
    }

    const float screenX = tvLeft + 29.0f * tvScale + 10.0f;
    const float screenY = tvTop + 66.0f * tvScale + 9.0f;
    const float screenW = 240.0f * tvScale - 20.0f;
    const float screenH = 137.0f * tvScale - 18.0f;
    sceneRenderer2D_.SetScissor(screenX, screenY, screenW, screenH);
    sceneRenderer2D_.DrawRect(screenX, screenY, screenW, screenH, Rgba(0.08f, 0.08f, 0.08f, 0.86f));
    DrawRectOutline(screenX + 3.0f, screenY + 3.0f, screenW - 6.0f, screenH - 6.0f, 2.0f, Rgba(0.34f, 0.34f, 0.34f));
    DrawTextCenteredFit(UiString("settings.title"), screenX + screenW * 0.5f, screenY + 19.0f, 23.0f, screenW - 24.0f, Rgba(1.0f, 1.0f, 1.0f));

    const std::array<const char*, 4> tabs{{"settings.tab.audio", "settings.tab.display", "settings.tab.visuals", "settings.tab.monitor"}};
    const float tabWidth = (screenW - 32.0f) / static_cast<float>(tabs.size());
    for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
        const bool active = i == uiState_.settings.section;
        const float tabX = screenX + 16.0f + static_cast<float>(i) * tabWidth;
        const float tabY = screenY + 31.0f;
        sceneRenderer2D_.DrawRect(tabX, tabY, tabWidth - 3.0f, 12.0f, active ? Rgba(0.32f, 0.32f, 0.32f, 0.92f) : Rgba(0.15f, 0.15f, 0.15f, 0.82f));
        DrawRectOutline(tabX, tabY, tabWidth - 3.0f, 12.0f, 1.0f, active ? Rgba(0.72f, 0.72f, 0.72f) : Rgba(0.31f, 0.31f, 0.31f));
        DrawTextCenteredFit(UiString(tabs[static_cast<std::size_t>(i)]), tabX + (tabWidth - 3.0f) * 0.5f, tabY + 6.0f, 8.5f, tabWidth - 7.0f, active ? Rgba(1.0f, 1.0f, 1.0f) : Rgba(0.64f, 0.64f, 0.64f));
    }
    sceneRenderer2D_.ClearScissor();

    const float rowsX = screenX + 10.0f;
    const float rowsY = screenY + 48.0f;
    const float rowsW = screenW - 20.0f;
    const float rowsH = screenH - 56.0f;
    sceneRenderer2D_.SetScissor(rowsX, rowsY, rowsW, rowsH);
    std::vector<VulkanUiRowState> fallbackRows;
    if (uiState_.settings.rows.empty()) {
        fallbackRows = {
            {"settings.category", "settings.tab.display", true, false, 0.0f, true, false, true},
            {"settings.language", "English", false, false, 0.0f, false, false, true},
            {"settings.fullscreen", "settings.value.off", true, false, 0.0f, false, false, false},
            {"settings.window_size", "960 x 540", false, false, 0.0f, false, false, true},
            {"settings.vsync", "settings.value.on", true, false, 0.0f, false, false, true},
            {"settings.output_smoothing", "settings.value.linear", true, false, 0.0f, false, false, true},
            {"settings.msaa", "4x", false, false, 0.0f, false, false, false},
            {"settings.master_volume", "", false, true, 0.72f, false, false, true},
            {"settings.back", "settings.value.esc", true, false, 0.0f, false, false, true}
        };
    }
    const std::vector<VulkanUiRowState>& rows = uiState_.settings.rows.empty() ? fallbackRows : uiState_.settings.rows;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const VulkanUiRowState& rowState = rows[static_cast<std::size_t>(i)];
        const float rowY = rowsY + static_cast<float>(i) * 16.0f - uiState_.settings.scrollOffset;
        if (rowY > rowsY + rowsH) {
            break;
        }
        if (rowY + 14.0f < rowsY) {
            continue;
        }
        const bool selected = rowState.selected;
        const bool hovered = rowState.hovered;
        if (selected || hovered) {
            sceneRenderer2D_.DrawRect(rowsX, rowY, rowsW, 14.0f, selected ? Rgba(0.33f, 0.33f, 0.33f, 0.92f) : Rgba(0.23f, 0.23f, 0.23f, 0.82f));
            DrawRectOutline(rowsX, rowY, rowsW, 14.0f, 1.0f, Rgba(0.72f, 0.72f, 0.72f));
        }
        const VulkanRenderer2D::Color textColor = !rowState.enabled
            ? Rgba(0.50f, 0.50f, 0.50f)
            : (selected || hovered ? Rgba(1.0f, 1.0f, 1.0f) : Rgba(0.86f, 0.86f, 0.86f));
        DrawText(rowState.labelKey, rowsX + 8.0f, rowY + 2.0f, 12.0f, textColor);
        if (rowState.slider) {
            const float sliderX = rowsX + rowsW - 108.0f;
            const float sliderY = rowY + 5.0f;
            sceneRenderer2D_.DrawRect(sliderX, sliderY, 94.0f, 4.0f, Rgba(0.44f, 0.44f, 0.44f));
            sceneRenderer2D_.DrawRect(sliderX, sliderY, 94.0f * rowState.sliderValue, 4.0f, Rgba(0.91f, 0.91f, 0.91f));
            sceneRenderer2D_.DrawRect(sliderX + 94.0f * rowState.sliderValue - 2.0f, sliderY - 4.0f, 4.0f, 12.0f, Rgba(1.0f, 1.0f, 1.0f));
        } else {
            const std::string value = rowState.valueIsLocalizationKey ? UiString(rowState.value) : rowState.value;
            const VulkanFont::TextExtent measured = uiFont_.MeasureText(value, 12.0f);
            DrawTextRaw(value, rowsX + rowsW - measured.width - 8.0f, rowY + 2.0f, 12.0f, textColor);
        }
    }
    if (!rows.empty() && static_cast<float>(rows.size()) * 16.0f > rowsH) {
        const float maxScroll = static_cast<float>(rows.size()) * 16.0f - rowsH;
        const float trackX = rowsX + rowsW - 5.0f;
        sceneRenderer2D_.DrawRect(trackX, rowsY + 2.0f, 2.0f, rowsH - 4.0f, Rgba(0.27f, 0.27f, 0.27f, 0.72f));
        const float thumbHeight = std::max(16.0f, rowsH * rowsH / (rowsH + maxScroll));
        const float thumbY = rowsY + (rowsH - thumbHeight) * (uiState_.settings.scrollOffset / std::max(1.0f, maxScroll));
        sceneRenderer2D_.DrawRect(trackX - 1.0f, thumbY, 4.0f, thumbHeight, Rgba(0.75f, 0.75f, 0.75f, 0.86f));
    }
    sceneRenderer2D_.ClearScissor();
}

void VulkanBackend::DrawSharedGameBoundaryScene() {
    if (uiBg_.descriptor != VK_NULL_HANDLE) {
        sceneRenderer2D_.DrawTexturedSourceRect(-100.0f,
                                                -100.0f,
                                                static_cast<float>(uiBg_.texture.Width()),
                                                static_cast<float>(uiBg_.texture.Height()),
                                                uiBg_.descriptor,
                                                uiBg_.texture.Width(),
                                                uiBg_.texture.Height(),
                                                {0.0f, 0.0f, static_cast<float>(uiBg_.texture.Width()), static_cast<float>(uiBg_.texture.Height())});
    } else {
        sceneRenderer2D_.DrawRect(0.0f, 0.0f, 960.0f, 540.0f, Rgba(0.05f, 0.05f, 0.055f));
    }

    sceneRenderer2D_.DrawRect(90.0f, 118.0f, 780.0f, 306.0f, Rgba(0.07f, 0.07f, 0.075f, 0.94f));
    DrawRectOutline(90.0f, 118.0f, 780.0f, 306.0f, 3.0f, Rgba(0.72f, 0.72f, 0.72f, 0.85f));
    sceneRenderer2D_.DrawRect(90.0f, 118.0f, 780.0f, 52.0f, Rgba(0.82f, 0.12f, 0.13f, 0.92f));
    DrawTextCentered(UiString("vulkan.shared_path.title"), 480.0f, 144.0f, 31.0f, Rgba(1.0f, 1.0f, 1.0f));
    DrawTextCentered(UiString("vulkan.shared_path.status"), 480.0f, 204.0f, 22.0f, Rgba(0.96f, 0.96f, 0.96f));
    DrawTextCentered(UiString("vulkan.shared_path.blocker"), 480.0f, 273.0f, 17.0f, Rgba(0.84f, 0.84f, 0.84f));
    DrawTextCentered(UiString("vulkan.shared_path.next"), 480.0f, 338.0f, 17.0f, Rgba(0.78f, 0.78f, 0.78f));
    DrawTextCentered(UiString("vulkan.shared_path.back"), 480.0f, 394.0f, 18.0f, Rgba(0.96f, 0.96f, 0.96f));
}

wb::render::TextureHandle VulkanBackend::LoadRendererTexture(const std::filesystem::path& path,
                                                              wb::render::TextureFilter filter,
                                                              wb::render::TextureSizing sizing) {
    if (!initialized_) {
        throw std::runtime_error("LoadRendererTexture called before VulkanBackend initialization");
    }
    if (path.empty()) {
        return {};
    }

    const bool renderResolution = sizing == wb::render::TextureSizing::RenderResolution;
    const LoadedImage image = LoadRgbaImage(
        path,
        renderResolution ? config_.renderWidth : 0u,
        renderResolution ? config_.renderHeight : 0u,
        filter == wb::render::TextureFilter::Nearest
    );
    auto texture = std::make_unique<RendererTexture>();
    texture->filter = filter;
    texture->texture.CreateRGBA8(device_,
                                  commandContext_.Pool(),
                                  device_.GraphicsQueue(),
                                  image.width,
                                  image.height,
                                  image.rgba.data(),
                                  ToVkFilter(filter));
    texture->descriptor = scenePipeline_.CreateTextureDescriptorSet(texture->texture.ImageView(),
                                                                    texture->texture.Sampler());

    const wb::render::TextureHandle handle{nextRendererTextureId_++};
    rendererTextures_[handle.id] = std::move(texture);
    return handle;
}

wb::render::TextureHandle VulkanBackend::CreateRendererTextureRGBA(const wb::render::TextureDesc& desc,
                                                                   const void* pixels) {
    if (!initialized_) {
        throw std::runtime_error("CreateRendererTextureRGBA called before VulkanBackend initialization");
    }
    if (pixels == nullptr || desc.width == 0 || desc.height == 0) {
        return {};
    }

    auto texture = std::make_unique<RendererTexture>();
    texture->filter = desc.filter;
    texture->texture.CreateRGBA8(device_,
                                  commandContext_.Pool(),
                                  device_.GraphicsQueue(),
                                  desc.width,
                                  desc.height,
                                  pixels,
                                  ToVkFilter(desc.filter));
    texture->descriptor = scenePipeline_.CreateTextureDescriptorSet(texture->texture.ImageView(),
                                                                    texture->texture.Sampler());

    const wb::render::TextureHandle handle{nextRendererTextureId_++};
    rendererTextures_[handle.id] = std::move(texture);
    return handle;
}

void VulkanBackend::DestroyRendererTexture(wb::render::TextureHandle texture) {
    auto ownedTarget = rendererTargetTextureOwners_.find(texture.id);
    if (ownedTarget != rendererTargetTextureOwners_.end()) {
        rendererTargetTextureOwners_.erase(ownedTarget);
        return;
    }

    auto it = rendererTextures_.find(texture.id);
    if (it == rendererTextures_.end()) {
        return;
    }
    if (it->second) {
        it->second->texture.Destroy(device_.Logical());
    }
    rendererTextures_.erase(it);
}

void VulkanBackend::SetRendererTextureFilter(wb::render::TextureHandle texture, wb::render::TextureFilter filter) {
    if (RendererTexture* value = FindRendererTexture(texture)) {
        if (value->filter == filter) {
            return;
        }
        WaitIdle();
        value->texture.SetFilter(device_.Logical(), ToVkFilter(filter));
        scenePipeline_.UpdateTextureDescriptorSet(value->descriptor,
                                                  value->texture.ImageView(),
                                                  value->texture.Sampler());
        value->filter = filter;
        return;
    }
    const auto owner = rendererTargetTextureOwners_.find(texture.id);
    if (owner != rendererTargetTextureOwners_.end()) {
        RendererTarget* target = FindRendererTarget(wb::render::RenderTargetHandle{owner->second});
        if (!target || target->filter == filter) {
            return;
        }
        WaitIdle();
        target->target.SetFilter(device_.Logical(), ToVkFilter(filter));
        scenePipeline_.UpdateTextureDescriptorSet(target->descriptor,
                                                  target->target.ImageView(),
                                                  target->target.Sampler());
        target->filter = filter;
    }
}

wb::render::Vec2 VulkanBackend::RendererTextureSize(wb::render::TextureHandle texture) const {
    if (const RendererTexture* value = FindRendererTexture(texture)) {
        return {static_cast<float>(value->texture.Width()), static_cast<float>(value->texture.Height())};
    }
    const auto ownerIt = rendererTargetTextureOwners_.find(texture.id);
    if (ownerIt != rendererTargetTextureOwners_.end()) {
        const RendererTarget* target = FindRendererTarget(wb::render::RenderTargetHandle{ownerIt->second});
        if (target) {
            const VkExtent2D extent = target->target.Extent();
            return {static_cast<float>(extent.width), static_cast<float>(extent.height)};
        }
    }
    return {};
}

wb::render::RenderTargetHandle VulkanBackend::CreateRendererRenderTarget(std::uint32_t width, std::uint32_t height) {
    if (!initialized_) {
        throw std::runtime_error("CreateRendererRenderTarget called before VulkanBackend initialization");
    }
    if (width == 0 || height == 0) {
        return {};
    }

    auto target = std::make_unique<RendererTarget>();
    target->filter = config_.outputLinearFilter ? wb::render::TextureFilter::Linear
                                                : wb::render::TextureFilter::Nearest;
    target->target.Create(device_,
                          sceneRenderPass_.Get(),
                          width,
                          height,
                          logicalTarget_.Format(),
                          config_.outputLinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
                          activeSampleCount_);
    target->descriptor = scenePipeline_.CreateTextureDescriptorSet(target->target.ImageView(),
                                                                  target->target.Sampler());

    const wb::render::RenderTargetHandle handle{nextRendererTargetId_++};
    rendererTargets_[handle.id] = std::move(target);
    return handle;
}

void VulkanBackend::DestroyRendererRenderTarget(wb::render::RenderTargetHandle target) {
    auto it = rendererTargets_.find(target.id);
    if (it == rendererTargets_.end()) {
        return;
    }
    for (auto alias = rendererTargetTextureOwners_.begin(); alias != rendererTargetTextureOwners_.end();) {
        if (alias->second == target.id) {
            alias = rendererTargetTextureOwners_.erase(alias);
        } else {
            ++alias;
        }
    }
    if (it->second) {
        it->second->target.Destroy(device_.Logical());
    }
    rendererTargets_.erase(it);
}

void VulkanBackend::BeginRendererRenderTarget(wb::render::RenderTargetHandle target) {
    RendererTarget* rendererTarget = FindRendererTarget(target);
    if (!rendererTarget) {
        return;
    }
    if (rendererTargetPassOpen_) {
        throw std::runtime_error("BeginRendererRenderTarget called while another render target is active");
    }
    if (frameOpen_ || logicalPassOpen_) {
        throw std::runtime_error("BeginRendererRenderTarget cannot be nested inside an active Vulkan frame");
    }

    activeRendererTargetCommandBuffer_ = BeginSingleUseCommands(device_.Logical(), commandContext_.Pool());

    VkClearValue clearColor{};
    clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = sceneRenderPass_.Get();
    renderPassInfo.framebuffer = rendererTarget->target.Framebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = rendererTarget->target.Extent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(activeRendererTargetCommandBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    sceneRenderer2D_.BeginFrame(activeRendererTargetCommandBuffer_,
                                scenePipeline_.Get(),
                                scenePipeline_.Layout(),
                                scenePipeline_.Textured(),
                                scenePipeline_.TexturedLayout(),
                                rendererTarget->target.Extent(),
                                currentFrame_ % kFramesInFlight);

    activeRendererTarget_ = target;
    logicalPassOpen_ = true;
    rendererTargetPassOpen_ = true;
}

void VulkanBackend::EndRendererRenderTarget() {
    if (!rendererTargetPassOpen_) {
        return;
    }

    RendererTarget* target = FindRendererTarget(activeRendererTarget_);
    ClearRendererScissor();
    sceneRenderer2D_.EndFrame();
    vkCmdEndRenderPass(activeRendererTargetCommandBuffer_);

    logicalPassOpen_ = false;
    rendererTargetPassOpen_ = false;

    EndSingleUseCommands(device_.Logical(),
                         commandContext_.Pool(),
                         device_.GraphicsQueue(),
                         activeRendererTargetCommandBuffer_);

    if (target) {
        target->hasContent = true;
    }
    activeRendererTargetCommandBuffer_ = VK_NULL_HANDLE;
    activeRendererTarget_ = {};
}

wb::render::TextureHandle VulkanBackend::RendererRenderTargetTexture(wb::render::RenderTargetHandle target) {
    if (!FindRendererTarget(target)) {
        return {};
    }
    for (const auto& [textureId, ownerId] : rendererTargetTextureOwners_) {
        if (ownerId == target.id) {
            return wb::render::TextureHandle{textureId};
        }
    }
    const wb::render::TextureHandle texture{nextRendererTextureId_++};
    rendererTargetTextureOwners_[texture.id] = target.id;
    return texture;
}

bool VulkanBackend::UpscaleRendererRenderTargetFsr1(wb::render::RenderTargetHandle source,
                                                    wb::render::RenderTargetHandle destination,
                                                    float sharpness) {
    if (!initialized_ || frameOpen_ || logicalPassOpen_ || rendererTargetPassOpen_) {
        return false;
    }

    const RendererTarget* sourceTarget = FindRendererTarget(source);
    RendererTarget* destinationTarget = FindRendererTarget(destination);
    if (!sourceTarget || !destinationTarget || !sourceTarget->hasContent || source.id == destination.id) {
        return false;
    }

    const VkExtent2D sourceExtent = sourceTarget->target.Extent();
    const VkExtent2D destinationExtent = destinationTarget->target.Extent();
    if (sourceExtent.width == 0 || sourceExtent.height == 0 ||
        destinationExtent.width <= sourceExtent.width || destinationExtent.height <= sourceExtent.height) {
        return false;
    }
    const double areaScale =
        (static_cast<double>(destinationExtent.width) * static_cast<double>(destinationExtent.height)) /
        (static_cast<double>(sourceExtent.width) * static_cast<double>(sourceExtent.height));
    if (areaScale > 4.001) {
        return false;
    }

    WaitIdle();
    if (fsrIntermediateTarget_.Extent().width != destinationExtent.width ||
        fsrIntermediateTarget_.Extent().height != destinationExtent.height ||
        fsrIntermediateTarget_.Format() != destinationTarget->target.Format()) {
        fsrIntermediateTarget_.Destroy(device_.Logical());
        fsrIntermediateTarget_.Create(device_,
                                      sceneRenderPass_.Get(),
                                      destinationExtent.width,
                                      destinationExtent.height,
                                      destinationTarget->target.Format(),
                                      VK_FILTER_LINEAR,
                                      activeSampleCount_);
    }

    const VkDescriptorSet sourceDescriptor = fsrEasuPipeline_.CreateTextureDescriptorSet(
        sourceTarget->target.ImageView(),
        sourceTarget->target.Sampler()
    );
    const VkDescriptorSet intermediateDescriptor = fsrRcasPipeline_.CreateTextureDescriptorSet(
        fsrIntermediateTarget_.ImageView(),
        fsrIntermediateTarget_.Sampler()
    );

    const VkCommandBuffer commandBuffer = BeginSingleUseCommands(device_.Logical(), commandContext_.Pool());
    auto recordPass = [&](VulkanRenderTarget& target,
                          VulkanPipeline& pipeline,
                          VulkanRenderer2D& renderer,
                          VkDescriptorSet descriptor,
                          const VulkanRenderer2D::PostProcessSettings& settings) {
        VkClearValue clearColor{};
        clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = sceneRenderPass_.Get();
        renderPassInfo.framebuffer = target.Framebuffer();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = target.Extent();
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        renderer.BeginFrame(commandBuffer,
                            pipeline.Get(),
                            pipeline.Layout(),
                            pipeline.Textured(),
                            pipeline.TexturedLayout(),
                            target.Extent(),
                            currentFrame_ % kFramesInFlight);
        renderer.SetPostProcessSettings(settings);
        renderer.DrawTexturedRect(0.0f,
                                  0.0f,
                                  static_cast<float>(target.Extent().width),
                                  static_cast<float>(target.Extent().height),
                                  descriptor);
        renderer.EndFrame();
        vkCmdEndRenderPass(commandBuffer);
    };

    VulkanRenderer2D::PostProcessSettings easuSettings;
    easuSettings.textureWidth = static_cast<float>(sourceExtent.width);
    easuSettings.textureHeight = static_cast<float>(sourceExtent.height);
    recordPass(fsrIntermediateTarget_,
               fsrEasuPipeline_,
               fsrEasuRenderer2D_,
               sourceDescriptor,
               easuSettings);

    VulkanRenderer2D::PostProcessSettings rcasSettings;
    rcasSettings.textureWidth = static_cast<float>(destinationExtent.width);
    rcasSettings.textureHeight = static_cast<float>(destinationExtent.height);
    rcasSettings.sharpness = std::clamp(sharpness, 0.0f, 2.0f);
    recordPass(destinationTarget->target,
               fsrRcasPipeline_,
               fsrRcasRenderer2D_,
               intermediateDescriptor,
               rcasSettings);

    EndSingleUseCommands(device_.Logical(),
                         commandContext_.Pool(),
                         device_.GraphicsQueue(),
                         commandBuffer);
    destinationTarget->hasContent = true;
    return true;
}

std::vector<std::uint8_t> VulkanBackend::CaptureRendererRenderTargetRGBA(wb::render::RenderTargetHandle target) {
    RendererTarget* rendererTarget = FindRendererTarget(target);
    if (!rendererTarget) {
        return {};
    }
    if (!rendererTarget->hasContent) {
        WarnUnsupportedOnce("renderer.capture_empty_render_target",
                            "Warning: Vulkan renderer render-target readback requested before the target has been drawn.\n");
        return {};
    }
    if (!IsRgbaFormat(rendererTarget->target.Format()) && !IsBgraFormat(rendererTarget->target.Format())) {
        WarnUnsupportedOnce("renderer.capture_format",
                            "Warning: Vulkan renderer render-target readback only supports RGBA8/BGRA8 targets right now.\n");
        return {};
    }

    const VkExtent2D extent = rendererTarget->target.Extent();
    const VkDeviceSize byteCount = static_cast<VkDeviceSize>(extent.width) *
                                   static_cast<VkDeviceSize>(extent.height) *
                                   4u;
    if (byteCount == 0) {
        return {};
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    CreateBuffer(device_,
                 byteCount,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingMemory);

    VkCommandBuffer commandBuffer = BeginSingleUseCommands(device_.Logical(), commandContext_.Pool());
    TransitionColorImageLayout(commandBuffer,
                               rendererTarget->target.Image(),
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               VK_ACCESS_SHADER_READ_BIT,
                               VK_ACCESS_TRANSFER_READ_BIT,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                               VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = {extent.width, extent.height, 1};
    vkCmdCopyImageToBuffer(commandBuffer,
                           rendererTarget->target.Image(),
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stagingBuffer,
                           1,
                           &copyRegion);

    TransitionColorImageLayout(commandBuffer,
                               rendererTarget->target.Image(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_ACCESS_TRANSFER_READ_BIT,
                               VK_ACCESS_SHADER_READ_BIT,
                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    EndSingleUseCommands(device_.Logical(), commandContext_.Pool(), device_.GraphicsQueue(), commandBuffer);

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(byteCount));
    void* mapped = nullptr;
    CheckVk(vkMapMemory(device_.Logical(), stagingMemory, 0, byteCount, 0, &mapped),
            "mapping Vulkan render-target readback buffer");
    std::memcpy(pixels.data(), mapped, pixels.size());
    vkUnmapMemory(device_.Logical(), stagingMemory);

    vkDestroyBuffer(device_.Logical(), stagingBuffer, nullptr);
    vkFreeMemory(device_.Logical(), stagingMemory, nullptr);

    if (IsBgraFormat(rendererTarget->target.Format())) {
        for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
            std::swap(pixels[i], pixels[i + 2]);
        }
    }
    return pixels;
}

wb::render::FontHandle VulkanBackend::LoadRendererFont(const std::filesystem::path& path, int size) {
    if (!initialized_) {
        throw std::runtime_error("LoadRendererFont called before VulkanBackend initialization");
    }

    std::filesystem::path fontPath = path;
    if (fontPath.empty()) {
        fontPath = config_.fontPath;
    }
    if (fontPath.empty()) {
        fontPath = "C:/Windows/Fonts/arial.ttf";
    }

    auto font = std::make_unique<RendererFont>();
    font->font.Create(device_, commandContext_.Pool(), device_.GraphicsQueue(), fontPath, std::max(1, size));
    font->descriptor = scenePipeline_.CreateTextureDescriptorSet(font->font.ImageView(), font->font.Sampler());

    const wb::render::FontHandle handle{nextRendererFontId_++};
    rendererFonts_[handle.id] = std::move(font);
    return handle;
}

void VulkanBackend::DestroyRendererFont(wb::render::FontHandle font) {
    auto it = rendererFonts_.find(font.id);
    if (it == rendererFonts_.end()) {
        return;
    }
    if (it->second) {
        it->second->font.Destroy(device_.Logical());
    }
    rendererFonts_.erase(it);
}

wb::render::Vec2 VulkanBackend::MeasureRendererText(wb::render::FontHandle font,
                                                    std::string_view text,
                                                    float size) const {
    const RendererFont* value = FindRendererFont(font);
    if (!value) {
        return {};
    }
    const VulkanFont::TextExtent measured = value->font.MeasureText(text, size);
    return {measured.width, measured.height};
}

void VulkanBackend::DrawRendererTexture(const wb::render::DrawTextureParams& params) {
    EnsureExternalDrawOpen("DrawTexture");

    VkDescriptorSet descriptor = VK_NULL_HANDLE;
    std::uint32_t textureWidth = 0;
    std::uint32_t textureHeight = 0;

    if (const RendererTexture* texture = FindRendererTexture(params.texture)) {
        descriptor = texture->descriptor;
        textureWidth = texture->texture.Width();
        textureHeight = texture->texture.Height();
    } else {
        const auto ownerIt = rendererTargetTextureOwners_.find(params.texture.id);
        if (ownerIt != rendererTargetTextureOwners_.end()) {
            const RendererTarget* target = FindRendererTarget(wb::render::RenderTargetHandle{ownerIt->second});
            if (target) {
                descriptor = target->descriptor;
                const VkExtent2D extent = target->target.Extent();
                textureWidth = extent.width;
                textureHeight = extent.height;
            }
        }
    }

    if (descriptor == VK_NULL_HANDLE || textureWidth == 0 || textureHeight == 0) {
        return;
    }

    wb::render::Rect source = params.source;
    if (source.width == 0.0f && source.height == 0.0f) {
        source = {0.0f, 0.0f, static_cast<float>(textureWidth), static_cast<float>(textureHeight)};
    }
    if (params.flipX) {
        source.x += source.width;
        source.width *= -1.0f;
    }
    if (params.flipY) {
        source.y += source.height;
        source.height *= -1.0f;
    }

    sceneRenderer2D_.DrawTexturedSourceRectTransformed(params.destination.x,
                                                       params.destination.y,
                                                       params.destination.width,
                                                       params.destination.height,
                                                       params.origin.x,
                                                       params.origin.y,
                                                       params.rotationRadians,
                                                       descriptor,
                                                       textureWidth,
                                                       textureHeight,
                                                       ToVulkanSource(source),
                                                       ToVulkanColor(params.tint, params.alpha));
}

void VulkanBackend::DrawRendererTextureQuad(const wb::render::DrawTextureQuadParams& params) {
    EnsureExternalDrawOpen("DrawTextureQuad");

    VkDescriptorSet descriptor = VK_NULL_HANDLE;
    std::uint32_t textureWidth = 0;
    std::uint32_t textureHeight = 0;

    if (const RendererTexture* texture = FindRendererTexture(params.texture)) {
        descriptor = texture->descriptor;
        textureWidth = texture->texture.Width();
        textureHeight = texture->texture.Height();
    } else {
        const auto ownerIt = rendererTargetTextureOwners_.find(params.texture.id);
        if (ownerIt != rendererTargetTextureOwners_.end()) {
            const RendererTarget* target = FindRendererTarget(wb::render::RenderTargetHandle{ownerIt->second});
            if (target) {
                descriptor = target->descriptor;
                const VkExtent2D extent = target->target.Extent();
                textureWidth = extent.width;
                textureHeight = extent.height;
            }
        }
    }

    if (descriptor == VK_NULL_HANDLE || textureWidth == 0 || textureHeight == 0) {
        return;
    }

    wb::render::Rect source = params.source;
    if (source.width == 0.0f && source.height == 0.0f) {
        source = {0.0f, 0.0f, static_cast<float>(textureWidth), static_cast<float>(textureHeight)};
    }
    if (params.flipX) {
        source.x += source.width;
        source.width *= -1.0f;
    }
    if (params.flipY) {
        source.y += source.height;
        source.height *= -1.0f;
    }

    const std::array<VulkanRenderer2D::Point, 4> vertices{{
        {params.vertices[0].x, params.vertices[0].y},
        {params.vertices[1].x, params.vertices[1].y},
        {params.vertices[2].x, params.vertices[2].y},
        {params.vertices[3].x, params.vertices[3].y}
    }};
    sceneRenderer2D_.DrawTexturedQuad(vertices,
                                      descriptor,
                                      textureWidth,
                                      textureHeight,
                                      ToVulkanSource(source),
                                      ToVulkanColor(params.tint, params.alpha));
}

void VulkanBackend::DrawRendererAtlasFrame(const wb::render::AtlasFrameParams& params) {
    if (params.rotated) {
        WarnUnsupportedOnce("renderer.rotated_atlas",
                            "Warning: Vulkan renderer rotated atlas frames are not implemented yet.\n");
        return;
    }

    const float absScaleX = std::abs(params.scale.x);
    const float absScaleY = std::abs(params.scale.y);
    wb::render::Rect source = params.frame;
    if (params.scale.x < 0.0f) {
        source.x += source.width;
        source.width *= -1.0f;
    }
    if (params.scale.y < 0.0f) {
        source.y += source.height;
        source.height *= -1.0f;
    }

    wb::render::DrawTextureParams draw{};
    draw.texture = params.texture;
    draw.source = source;
    draw.destination = {
        params.position.x,
        params.position.y,
        params.logicalFrameSize.x * absScaleX,
        params.logicalFrameSize.y * absScaleY
    };
    draw.origin = {
        (params.anchor.x * params.sourceSize.x - params.spriteSourceSize.x) * absScaleX,
        (params.anchor.y * params.sourceSize.y - params.spriteSourceSize.y) * absScaleY
    };
    if (params.scale.x < 0.0f) {
        draw.origin.x = draw.destination.width - draw.origin.x;
    }
    if (params.scale.y < 0.0f) {
        draw.origin.y = draw.destination.height - draw.origin.y;
    }
    draw.rotationRadians = params.rotationRadians;
    draw.tint = params.tint;
    draw.alpha = params.alpha;
    DrawRendererTexture(draw);
}

void VulkanBackend::DrawRendererRectangle(wb::render::Rect rect, wb::render::Color color, float alpha) {
    EnsureExternalDrawOpen("DrawRectangle");
    sceneRenderer2D_.DrawRect(rect.x, rect.y, rect.width, rect.height, ToVulkanColor(color, alpha));
}

void VulkanBackend::DrawRendererRectangleOutline(wb::render::Rect rect,
                                                 float thickness,
                                                 wb::render::Color color,
                                                 float alpha) {
    EnsureExternalDrawOpen("DrawRectangleOutline");
    DrawRectOutline(rect.x, rect.y, rect.width, rect.height, thickness, ToVulkanColor(color, alpha));
}

void VulkanBackend::DrawRendererLine(wb::render::Vec2 a,
                                     wb::render::Vec2 b,
                                     float thickness,
                                     wb::render::Color color,
                                     float alpha) {
    EnsureExternalDrawOpen("DrawLine");
    sceneRenderer2D_.DrawLine(a.x, a.y, b.x, b.y, thickness, ToVulkanColor(color, alpha));
}

void VulkanBackend::DrawRendererCircle(wb::render::Vec2 center, float radius, wb::render::Color color, float alpha) {
    EnsureExternalDrawOpen("DrawCircle");
    sceneRenderer2D_.DrawCircle(center.x, center.y, radius, ToVulkanColor(color, alpha));
}

void VulkanBackend::DrawRendererText(const wb::render::TextParams& params) {
    EnsureExternalDrawOpen("DrawText");
    const RendererFont* font = FindRendererFont(params.font);
    if (!font || params.text.empty()) {
        return;
    }
    font->font.DrawText(sceneRenderer2D_,
                        font->descriptor,
                        params.text,
                        params.position.x,
                        params.position.y,
                        params.size,
                        ToVulkanColor(params.color, params.alpha));
}

void VulkanBackend::PushRendererScissor(wb::render::Rect logicalRect) {
    EnsureExternalDrawOpen("PushScissor");
    rendererScissorStack_.push_back(logicalRect);
    sceneRenderer2D_.SetScissor(logicalRect.x, logicalRect.y, logicalRect.width, logicalRect.height);
}

void VulkanBackend::PopRendererScissor() {
    if (rendererScissorStack_.empty()) {
        ClearRendererScissor();
        return;
    }
    rendererScissorStack_.pop_back();
    if (rendererScissorStack_.empty()) {
        ClearRendererScissor();
        return;
    }
    const wb::render::Rect& rect = rendererScissorStack_.back();
    sceneRenderer2D_.SetScissor(rect.x, rect.y, rect.width, rect.height);
}

void VulkanBackend::ClearRendererScissor() {
    rendererScissorStack_.clear();
    if (logicalPassOpen_) {
        sceneRenderer2D_.ClearScissor();
    }
}

VulkanBackend::RendererTexture* VulkanBackend::FindRendererTexture(wb::render::TextureHandle texture) {
    const auto it = rendererTextures_.find(texture.id);
    return it != rendererTextures_.end() ? it->second.get() : nullptr;
}

const VulkanBackend::RendererTexture* VulkanBackend::FindRendererTexture(wb::render::TextureHandle texture) const {
    const auto it = rendererTextures_.find(texture.id);
    return it != rendererTextures_.end() ? it->second.get() : nullptr;
}

VulkanBackend::RendererFont* VulkanBackend::FindRendererFont(wb::render::FontHandle font) {
    const auto it = rendererFonts_.find(font.id);
    return it != rendererFonts_.end() ? it->second.get() : nullptr;
}

const VulkanBackend::RendererFont* VulkanBackend::FindRendererFont(wb::render::FontHandle font) const {
    const auto it = rendererFonts_.find(font.id);
    return it != rendererFonts_.end() ? it->second.get() : nullptr;
}

VulkanBackend::RendererTarget* VulkanBackend::FindRendererTarget(wb::render::RenderTargetHandle target) {
    const auto it = rendererTargets_.find(target.id);
    return it != rendererTargets_.end() ? it->second.get() : nullptr;
}

const VulkanBackend::RendererTarget* VulkanBackend::FindRendererTarget(wb::render::RenderTargetHandle target) const {
    const auto it = rendererTargets_.find(target.id);
    return it != rendererTargets_.end() ? it->second.get() : nullptr;
}

void VulkanBackend::WarnUnsupportedOnce(const char* key, const char* message) const {
    const std::string keyString = key != nullptr ? key : "";
    if (unsupportedWarnings_[keyString]) {
        return;
    }
    unsupportedWarnings_[keyString] = true;
    std::cerr << message;
}

void VulkanBackend::EnsureExternalDrawOpen(const char* action) const {
    if (!logicalPassOpen_) {
        throw std::runtime_error(std::string(action) + " called outside Vulkan logical render pass");
    }
}

void VulkanBackend::EndFrame() {
    if (!frameOpen_) {
        throw std::runtime_error("EndFrame called before BeginFrame");
    }
    if (logicalPassOpen_) {
        throw std::runtime_error("EndFrame called while a logical render pass is still open");
    }
    if (!clearRecorded_ && !logicalPassRecorded_) {
        ClearFrame();
    } else if (!presentPassRecorded_) {
        Present();
    }

    commandContext_.EndFrame(currentFrame_);

    VkSemaphore waitSemaphores[] = {sync_.ImageAvailable(currentFrame_)};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {sync_.RenderFinished(currentImageIndex_)};
    VkCommandBuffer commandBuffers[] = {commandContext_.Buffer(currentFrame_)};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffers;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    CheckVk(vkQueueSubmit(device_.GraphicsQueue(), 1, &submitInfo, sync_.InFlight(currentFrame_)),
            "submitting Vulkan command buffer");

    frameOpen_ = false;
}

bool VulkanBackend::PresentFrame() {
    if (frameOpen_) {
        throw std::runtime_error("PresentFrame called before EndFrame");
    }

    VkSemaphore signalSemaphores[] = {sync_.RenderFinished(currentImageIndex_)};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain_.Get()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex_;

    VkResult result = vkQueuePresentKHR(device_.PresentQueue(), &presentInfo);
    currentFrame_ = (currentFrame_ + 1) % kFramesInFlight;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;
    }
    if (result != VK_SUCCESS) {
        throw std::runtime_error(MakeVkErrorMessage(result, "presenting Vulkan swapchain image"));
    }
    return true;
}

void VulkanBackend::SetClearColor(float r, float g, float b, float a) noexcept {
    config_.clearColor = {r, g, b, a};
}

VulkanBackendStats VulkanBackend::Stats() const noexcept {
    VulkanBackendStats stats;
    stats.swapchainImageCount = static_cast<std::uint32_t>(swapchain_.Images().size());
    stats.swapchainFormat = swapchain_.ImageFormat();
    stats.swapchainColorSpace = swapchain_.ColorSpace();
    stats.presentMode = swapchain_.PresentMode();
    stats.swapchainExtent = swapchain_.Extent();
    stats.renderTargetExtent = logicalTarget_.Extent();
    stats.hdrSwapchainAvailable = swapchain_.HdrAvailable();
    stats.hdrActive = swapchain_.HdrActive();
    stats.hdrSystemEnabled = displayHdrState_.hdrEnabled;
    stats.hdrMetadataAvailable = device_.HdrMetadataEnabled();
    stats.hdrPaperWhiteNits = EffectiveHdrPaperWhiteNits();
    stats.hdrPeakNits = EffectiveHdrPeakNits();
    return stats;
}

} // namespace wbwwb::vulkan
