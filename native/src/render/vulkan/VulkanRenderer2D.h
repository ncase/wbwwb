#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

namespace wbwwb::vulkan {

class VulkanRenderer2D final {
public:
    struct Color {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct PostProcessSettings {
        float textureWidth = 1.0f;
        float textureHeight = 1.0f;
        float time = 0.0f;
        float useFxaa = 0.0f;
        float crtStrength = 0.0f;
        float noiseAmount = 0.0f;
        float brightness = 1.0f;
        float contrast = 1.0f;
        float sharpness = 1.0f;
        float gamma = 1.0f;
        float blackLevel = 0.0f;
        float whiteLevel = 1.0f;
        float screenBorder = 0.0f;
        float scanlines = 0.0f;
        float crtCurve = 0.0f;
        float hdrEncoding = 0.0f;
        float hdrPaperWhiteNits = 203.0f;
        float hdrMaxNits = 1000.0f;
        float hdrHighlightStrength = 0.35f;
    };

    void Create(const VulkanDevice& device, std::uint32_t maxVertices = 65536);
    void Destroy();

    void BeginFrame(VkCommandBuffer commandBuffer,
                    VkPipeline pipeline,
                    VkPipelineLayout pipelineLayout,
                    VkPipeline texturedPipeline,
                    VkPipelineLayout texturedPipelineLayout,
                    VkExtent2D viewportExtent,
                    std::uint32_t frameIndex,
                    VkExtent2D logicalExtent = {});
    void DrawRect(float x, float y, float width, float height, Color color);
    void DrawLine(float x0, float y0, float x1, float y1, float thickness, Color color);
    void DrawCircle(float centerX, float centerY, float radius, Color color, int segments = 32);
    struct SourceRect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    struct Point {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct AtlasFrame {
        SourceRect frame;
        SourceRect spriteSourceSize;
        float sourceWidth = 0.0f;
        float sourceHeight = 0.0f;
        bool rotated = false;
    };

    void DrawTexturedRect(float x,
                          float y,
                          float width,
                          float height,
                          VkDescriptorSet textureDescriptorSet,
                          Color color = {});
    void DrawTexturedSourceRect(float x,
                                float y,
                                float width,
                                float height,
                                VkDescriptorSet textureDescriptorSet,
                                std::uint32_t textureWidth,
                                std::uint32_t textureHeight,
                                SourceRect source,
                                Color color = {});
    void DrawTexturedSourceRectTransformed(float x,
                                           float y,
                                           float width,
                                           float height,
                                           float originX,
                                           float originY,
                                           float rotationRadians,
                                           VkDescriptorSet textureDescriptorSet,
                                           std::uint32_t textureWidth,
                                           std::uint32_t textureHeight,
                                           SourceRect source,
                                           Color color = {});
    void DrawTexturedQuad(const std::array<Point, 4>& vertices,
                          VkDescriptorSet textureDescriptorSet,
                          std::uint32_t textureWidth,
                          std::uint32_t textureHeight,
                          SourceRect source,
                          Color color = {});
    void DrawAtlasFrame(float x,
                        float y,
                        float scaleX,
                        float scaleY,
                        float anchorX,
                        float anchorY,
                        VkDescriptorSet textureDescriptorSet,
                        std::uint32_t textureWidth,
                        std::uint32_t textureHeight,
                        const AtlasFrame& frame,
                        Color color = {});
    void DrawAtlasFrameTransformed(float x,
                                   float y,
                                   float scaleX,
                                   float scaleY,
                                   float anchorX,
                                   float anchorY,
                                   float rotationRadians,
                                   VkDescriptorSet textureDescriptorSet,
                                   std::uint32_t textureWidth,
                                   std::uint32_t textureHeight,
                                   const AtlasFrame& frame,
                                   Color color = {});
    void SetScissor(float x, float y, float width, float height);
    void ClearScissor();
    void SetPostProcessSettings(const PostProcessSettings& settings);
    void EndFrame();

private:
    struct Vertex {
        float x = 0.0f;
        float y = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
        float uvMinX = 0.0f;
        float uvMinY = 0.0f;
        float uvMaxX = 1.0f;
        float uvMaxY = 1.0f;
    };

    struct FrameBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    void CreateVertexBuffer(const VulkanDevice& device, FrameBuffer& buffer);
    void Flush();
    void UploadAndDraw();
    void AppendQuad(std::vector<Vertex>& target,
                    float x,
                    float y,
                    float width,
                    float height,
                    float u0,
                    float v0,
                    float u1,
                    float v1,
                    Color color,
                    float uvMinX = 0.0f,
                    float uvMinY = 0.0f,
                    float uvMaxX = 1.0f,
                    float uvMaxY = 1.0f);
    void AppendTransformedQuad(std::vector<Vertex>& target,
                               float x,
                               float y,
                               float width,
                               float height,
                               float originX,
                               float originY,
                               float rotationRadians,
                               float u0,
                               float v0,
                               float u1,
                               float v1,
                               Color color,
                               float uvMinX,
                               float uvMinY,
                               float uvMaxX,
                               float uvMaxY);

    VkDevice device_ = VK_NULL_HANDLE;
    std::array<FrameBuffer, kFramesInFlight> vertexBuffers_{};
    std::vector<Vertex> solidVertices_;
    std::vector<Vertex> texturedVertices_;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline texturedPipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout texturedPipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet textureDescriptorSet_ = VK_NULL_HANDLE;
    VkExtent2D viewportExtent_{};
    VkExtent2D logicalExtent_{};
    VkRect2D scissor_{};
    bool scissorEnabled_ = false;
    std::uint32_t frameIndex_ = 0;
    std::uint32_t maxVertices_ = 0;
    std::uint32_t uploadedVertices_ = 0;
    PostProcessSettings postProcess_{};
};

} // namespace wbwwb::vulkan
