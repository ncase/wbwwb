#include "VulkanRenderer2D.h"

#include "VulkanErrors.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace wbwwb::vulkan {

void VulkanRenderer2D::Create(const VulkanDevice& device, std::uint32_t maxVertices) {
    device_ = device.Logical();
    maxVertices_ = maxVertices;
    solidVertices_.reserve(std::min<std::uint32_t>(maxVertices_, 4096));
    texturedVertices_.reserve(std::min<std::uint32_t>(maxVertices_, 4096));

    for (FrameBuffer& buffer : vertexBuffers_) {
        CreateVertexBuffer(device, buffer);
    }
}

void VulkanRenderer2D::CreateVertexBuffer(const VulkanDevice& device, FrameBuffer& buffer) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * maxVertices_;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CheckVk(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer.buffer),
            "creating Vulkan 2D vertex buffer");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device_, buffer.buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(
        memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CheckVk(vkAllocateMemory(device_, &allocInfo, nullptr, &buffer.memory),
            "allocating Vulkan 2D vertex buffer memory");
    CheckVk(vkBindBufferMemory(device_, buffer.buffer, buffer.memory, 0),
            "binding Vulkan 2D vertex buffer memory");
}

void VulkanRenderer2D::Destroy() {
    for (FrameBuffer& buffer : vertexBuffers_) {
        if (buffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, buffer.buffer, nullptr);
            buffer.buffer = VK_NULL_HANDLE;
        }
        if (buffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device_, buffer.memory, nullptr);
            buffer.memory = VK_NULL_HANDLE;
        }
    }
    solidVertices_.clear();
    texturedVertices_.clear();
    commandBuffer_ = VK_NULL_HANDLE;
    pipeline_ = VK_NULL_HANDLE;
    pipelineLayout_ = VK_NULL_HANDLE;
    texturedPipeline_ = VK_NULL_HANDLE;
    texturedPipelineLayout_ = VK_NULL_HANDLE;
    textureDescriptorSet_ = VK_NULL_HANDLE;
    viewportExtent_ = {};
    logicalExtent_ = {};
    scissor_ = {};
    scissorEnabled_ = false;
    frameIndex_ = 0;
    maxVertices_ = 0;
    uploadedVertices_ = 0;
    device_ = VK_NULL_HANDLE;
}

void VulkanRenderer2D::BeginFrame(VkCommandBuffer commandBuffer,
                                  VkPipeline pipeline,
                                  VkPipelineLayout pipelineLayout,
                                  VkPipeline texturedPipeline,
                                  VkPipelineLayout texturedPipelineLayout,
                                  VkExtent2D viewportExtent,
                                  std::uint32_t frameIndex,
                                  VkExtent2D logicalExtent) {
    commandBuffer_ = commandBuffer;
    pipeline_ = pipeline;
    pipelineLayout_ = pipelineLayout;
    texturedPipeline_ = texturedPipeline;
    texturedPipelineLayout_ = texturedPipelineLayout;
    viewportExtent_ = viewportExtent;
    logicalExtent_ = {
        logicalExtent.width > 0 ? logicalExtent.width : viewportExtent.width,
        logicalExtent.height > 0 ? logicalExtent.height : viewportExtent.height
    };
    frameIndex_ = frameIndex;
    textureDescriptorSet_ = VK_NULL_HANDLE;
    solidVertices_.clear();
    texturedVertices_.clear();
    scissor_ = {};
    scissorEnabled_ = false;
    uploadedVertices_ = 0;
}

void VulkanRenderer2D::DrawRect(float x, float y, float width, float height, Color color) {
    if (width <= 0.0f || height <= 0.0f || color.a <= 0.0f) {
        return;
    }
    if (!texturedVertices_.empty()) {
        Flush();
    }
    if (uploadedVertices_ + solidVertices_.size() + texturedVertices_.size() + 6 > maxVertices_) {
        throw std::runtime_error("Vulkan 2D vertex buffer capacity exceeded");
    }

    AppendQuad(solidVertices_, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

void VulkanRenderer2D::DrawLine(float x0, float y0, float x1, float y1, float thickness, Color color) {
    if (thickness <= 0.0f || color.a <= 0.0f) {
        return;
    }

    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.0001f) {
        DrawRect(x0 - thickness * 0.5f, y0 - thickness * 0.5f, thickness, thickness, color);
        return;
    }
    if (!texturedVertices_.empty()) {
        Flush();
    }
    if (uploadedVertices_ + solidVertices_.size() + texturedVertices_.size() + 6 > maxVertices_) {
        throw std::runtime_error("Vulkan 2D vertex buffer capacity exceeded");
    }

    const float nx = -dy / length * thickness * 0.5f;
    const float ny = dx / length * thickness * 0.5f;
    const Vertex a{x0 + nx, y0 + ny, 0.0f, 0.0f, color.r, color.g, color.b, color.a};
    const Vertex b{x1 + nx, y1 + ny, 0.0f, 0.0f, color.r, color.g, color.b, color.a};
    const Vertex c{x1 - nx, y1 - ny, 0.0f, 0.0f, color.r, color.g, color.b, color.a};
    const Vertex d{x0 - nx, y0 - ny, 0.0f, 0.0f, color.r, color.g, color.b, color.a};

    solidVertices_.push_back(a);
    solidVertices_.push_back(b);
    solidVertices_.push_back(c);
    solidVertices_.push_back(a);
    solidVertices_.push_back(c);
    solidVertices_.push_back(d);
}

void VulkanRenderer2D::DrawCircle(float centerX, float centerY, float radius, Color color, int segments) {
    if (radius <= 0.0f || color.a <= 0.0f) {
        return;
    }
    segments = std::clamp(segments, 8, 96);
    if (!texturedVertices_.empty()) {
        Flush();
    }
    const std::uint32_t vertexCount = static_cast<std::uint32_t>(segments * 3);
    if (uploadedVertices_ + solidVertices_.size() + texturedVertices_.size() + vertexCount > maxVertices_) {
        throw std::runtime_error("Vulkan 2D vertex buffer capacity exceeded");
    }

    constexpr float twoPi = 6.28318530717958647692f;
    for (int i = 0; i < segments; ++i) {
        const float a0 = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        const float a1 = twoPi * static_cast<float>(i + 1) / static_cast<float>(segments);
        solidVertices_.push_back({centerX, centerY, 0.0f, 0.0f, color.r, color.g, color.b, color.a});
        solidVertices_.push_back({
            centerX + std::cos(a0) * radius,
            centerY + std::sin(a0) * radius,
            0.0f,
            0.0f,
            color.r,
            color.g,
            color.b,
            color.a
        });
        solidVertices_.push_back({
            centerX + std::cos(a1) * radius,
            centerY + std::sin(a1) * radius,
            0.0f,
            0.0f,
            color.r,
            color.g,
            color.b,
            color.a
        });
    }
}

void VulkanRenderer2D::DrawTexturedRect(float x,
                                        float y,
                                        float width,
                                        float height,
                                        VkDescriptorSet textureDescriptorSet,
                                        Color color) {
    DrawTexturedSourceRect(x,
                           y,
                           width,
                           height,
                           textureDescriptorSet,
                           1,
                           1,
                           {0.0f, 0.0f, 1.0f, 1.0f},
                           color);
}

void VulkanRenderer2D::DrawTexturedSourceRect(float x,
                                              float y,
                                              float width,
                                              float height,
                                              VkDescriptorSet textureDescriptorSet,
                                              std::uint32_t textureWidth,
                                              std::uint32_t textureHeight,
                                              SourceRect source,
                                              Color color) {
    DrawTexturedSourceRectTransformed(x,
                                      y,
                                      width,
                                      height,
                                      0.0f,
                                      0.0f,
                                      0.0f,
                                      textureDescriptorSet,
                                      textureWidth,
                                      textureHeight,
                                      source,
                                      color);
}

void VulkanRenderer2D::DrawTexturedSourceRectTransformed(float x,
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
                                                         Color color) {
    if (width <= 0.0f || height <= 0.0f || color.a <= 0.0f) {
        return;
    }
    if (textureDescriptorSet == VK_NULL_HANDLE) {
        throw std::runtime_error("Vulkan textured draw requested with a null descriptor set");
    }
    if (textureWidth == 0 || textureHeight == 0 || source.width == 0.0f || source.height == 0.0f) {
        throw std::runtime_error("Vulkan textured draw requested with an empty source rectangle");
    }
    if (!solidVertices_.empty()) {
        Flush();
    }
    if (textureDescriptorSet_ != VK_NULL_HANDLE && textureDescriptorSet_ != textureDescriptorSet) {
        Flush();
    }
    if (uploadedVertices_ + solidVertices_.size() + texturedVertices_.size() + 6 > maxVertices_) {
        throw std::runtime_error("Vulkan 2D vertex buffer capacity exceeded");
    }

    textureDescriptorSet_ = textureDescriptorSet;
    const float inverseWidth = 1.0f / static_cast<float>(textureWidth);
    const float inverseHeight = 1.0f / static_cast<float>(textureHeight);
    const float u0 = source.x * inverseWidth;
    const float v0 = source.y * inverseHeight;
    const float u1 = (source.x + source.width) * inverseWidth;
    const float v1 = (source.y + source.height) * inverseHeight;
    const float uvMinX = (std::min(source.x, source.x + source.width) + 0.5f) * inverseWidth;
    const float uvMinY = (std::min(source.y, source.y + source.height) + 0.5f) * inverseHeight;
    const float uvMaxX = (std::max(source.x, source.x + source.width) - 0.5f) * inverseWidth;
    const float uvMaxY = (std::max(source.y, source.y + source.height) - 0.5f) * inverseHeight;
    AppendTransformedQuad(texturedVertices_,
                          x,
                          y,
                          width,
                          height,
                          originX,
                          originY,
                          rotationRadians,
                          u0,
                          v0,
                          u1,
                          v1,
                          color,
                          uvMinX,
                          uvMinY,
                          uvMaxX,
                          uvMaxY);
}

void VulkanRenderer2D::DrawTexturedQuad(const std::array<Point, 4>& vertices,
                                        VkDescriptorSet textureDescriptorSet,
                                        std::uint32_t textureWidth,
                                        std::uint32_t textureHeight,
                                        SourceRect source,
                                        Color color) {
    if (color.a <= 0.0f) {
        return;
    }
    if (textureDescriptorSet == VK_NULL_HANDLE) {
        throw std::runtime_error("Vulkan textured quad requested with a null descriptor set");
    }
    if (textureWidth == 0 || textureHeight == 0 || source.width == 0.0f || source.height == 0.0f) {
        throw std::runtime_error("Vulkan textured quad requested with an empty source rectangle");
    }
    if (!solidVertices_.empty()) {
        Flush();
    }
    if (textureDescriptorSet_ != VK_NULL_HANDLE && textureDescriptorSet_ != textureDescriptorSet) {
        Flush();
    }
    if (uploadedVertices_ + solidVertices_.size() + texturedVertices_.size() + 6 > maxVertices_) {
        throw std::runtime_error("Vulkan 2D vertex buffer capacity exceeded");
    }

    textureDescriptorSet_ = textureDescriptorSet;
    const float inverseWidth = 1.0f / static_cast<float>(textureWidth);
    const float inverseHeight = 1.0f / static_cast<float>(textureHeight);
    const float u0 = source.x * inverseWidth;
    const float v0 = source.y * inverseHeight;
    const float u1 = (source.x + source.width) * inverseWidth;
    const float v1 = (source.y + source.height) * inverseHeight;
    const float uvMinX = (std::min(source.x, source.x + source.width) + 0.5f) * inverseWidth;
    const float uvMinY = (std::min(source.y, source.y + source.height) + 0.5f) * inverseHeight;
    const float uvMaxX = (std::max(source.x, source.x + source.width) - 0.5f) * inverseWidth;
    const float uvMaxY = (std::max(source.y, source.y + source.height) - 0.5f) * inverseHeight;

    const Vertex tl{vertices[0].x, vertices[0].y, u0, v0, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};
    const Vertex bl{vertices[1].x, vertices[1].y, u0, v1, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};
    const Vertex br{vertices[2].x, vertices[2].y, u1, v1, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};
    const Vertex tr{vertices[3].x, vertices[3].y, u1, v0, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};

    texturedVertices_.push_back(tl);
    texturedVertices_.push_back(tr);
    texturedVertices_.push_back(br);
    texturedVertices_.push_back(tl);
    texturedVertices_.push_back(br);
    texturedVertices_.push_back(bl);
}

void VulkanRenderer2D::DrawAtlasFrame(float x,
                                      float y,
                                      float scaleX,
                                      float scaleY,
                                      float anchorX,
                                      float anchorY,
                                      VkDescriptorSet textureDescriptorSet,
                                      std::uint32_t textureWidth,
                                      std::uint32_t textureHeight,
                                      const AtlasFrame& frame,
                                      Color color) {
    DrawAtlasFrameTransformed(x,
                              y,
                              scaleX,
                              scaleY,
                              anchorX,
                              anchorY,
                              0.0f,
                              textureDescriptorSet,
                              textureWidth,
                              textureHeight,
                              frame,
                              color);
}

void VulkanRenderer2D::DrawAtlasFrameTransformed(float x,
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
                                                 Color color) {
    if (frame.rotated) {
        throw std::runtime_error("Vulkan atlas frame drawing does not support rotated frames");
    }
    if (frame.frame.width <= 0.0f || frame.frame.height <= 0.0f ||
        frame.sourceWidth <= 0.0f || frame.sourceHeight <= 0.0f) {
        return;
    }

    const float absScaleX = std::abs(scaleX);
    const float absScaleY = std::abs(scaleY);
    if (absScaleX <= 0.0f || absScaleY <= 0.0f) {
        return;
    }

    const float destinationWidth = frame.frame.width * absScaleX;
    const float destinationHeight = frame.frame.height * absScaleY;
    float originX = ((anchorX * frame.sourceWidth) - frame.spriteSourceSize.x) * absScaleX;
    float originY = ((anchorY * frame.sourceHeight) - frame.spriteSourceSize.y) * absScaleY;

    SourceRect source = frame.frame;
    if (scaleX < 0.0f) {
        source.width = -source.width;
        originX = destinationWidth - originX;
    }
    if (scaleY < 0.0f) {
        source.height = -source.height;
        originY = destinationHeight - originY;
    }

    DrawTexturedSourceRectTransformed(x,
                                      y,
                                      destinationWidth,
                                      destinationHeight,
                                      originX,
                                      originY,
                                      rotationRadians,
                                      textureDescriptorSet,
                                      textureWidth,
                                      textureHeight,
                                      source,
                                      color);
}

void VulkanRenderer2D::SetScissor(float x, float y, float width, float height) {
    Flush();

    const float logicalWidth = static_cast<float>(std::max(1u, logicalExtent_.width));
    const float logicalHeight = static_cast<float>(std::max(1u, logicalExtent_.height));
    const float targetWidth = static_cast<float>(viewportExtent_.width);
    const float targetHeight = static_cast<float>(viewportExtent_.height);
    const float scaleX = targetWidth / logicalWidth;
    const float scaleY = targetHeight / logicalHeight;
    const float left = std::clamp(x, 0.0f, logicalWidth) * scaleX;
    const float right = std::clamp(x + std::max(width, 0.0f), 0.0f, logicalWidth) * scaleX;
    const float logicalTop = std::clamp(y, 0.0f, logicalHeight) * scaleY;
    const float logicalBottom = std::clamp(y + std::max(height, 0.0f), 0.0f, logicalHeight) * scaleY;

    // Scene vertices use a top-left logical origin and are flipped back during
    // presentation. Vulkan scissors address the intermediate framebuffer, so
    // their Y interval must receive the same vertical conversion.
    const float framebufferTop = targetHeight - logicalBottom;
    const float framebufferBottom = targetHeight - logicalTop;
    const float roundedLeft = std::floor(left);
    const float roundedTop = std::floor(framebufferTop);
    const float roundedRight = std::ceil(right);
    const float roundedBottom = std::ceil(framebufferBottom);

    scissor_.offset.x = static_cast<std::int32_t>(roundedLeft);
    scissor_.offset.y = static_cast<std::int32_t>(roundedTop);
    scissor_.extent.width = static_cast<std::uint32_t>(std::max(0.0f, roundedRight - roundedLeft));
    scissor_.extent.height = static_cast<std::uint32_t>(std::max(0.0f, roundedBottom - roundedTop));
    scissorEnabled_ = true;
}

void VulkanRenderer2D::ClearScissor() {
    Flush();
    scissor_ = {};
    scissorEnabled_ = false;
}

void VulkanRenderer2D::SetPostProcessSettings(const PostProcessSettings& settings) {
    Flush();
    postProcess_ = settings;
}

void VulkanRenderer2D::AppendQuad(std::vector<Vertex>& target,
                                  float x,
                                  float y,
                                  float width,
                                  float height,
                                  float u0,
                                  float v0,
                                  float u1,
                                  float v1,
                                  Color color,
                                  float uvMinX,
                                  float uvMinY,
                                  float uvMaxX,
                                  float uvMaxY) {
    const float right = x + width;
    const float bottom = y + height;
    const Vertex tl{x, y, u0, v0, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};
    const Vertex tr{right, y, u1, v0, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};
    const Vertex br{right, bottom, u1, v1, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};
    const Vertex bl{x, bottom, u0, v1, color.r, color.g, color.b, color.a, uvMinX, uvMinY, uvMaxX, uvMaxY};

    target.push_back(tl);
    target.push_back(tr);
    target.push_back(br);
    target.push_back(tl);
    target.push_back(br);
    target.push_back(bl);
}

void VulkanRenderer2D::AppendTransformedQuad(std::vector<Vertex>& target,
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
                                             float uvMaxY) {
    if (std::abs(rotationRadians) <= 0.000001f && originX == 0.0f && originY == 0.0f) {
        AppendQuad(target, x, y, width, height, u0, v0, u1, v1, color, uvMinX, uvMinY, uvMaxX, uvMaxY);
        return;
    }

    const float cosR = std::cos(rotationRadians);
    const float sinR = std::sin(rotationRadians);
    auto transform = [&](float localX, float localY, float u, float v) {
        const float px = localX - originX;
        const float py = localY - originY;
        return Vertex{
            x + px * cosR - py * sinR,
            y + px * sinR + py * cosR,
            u,
            v,
            color.r,
            color.g,
            color.b,
            color.a,
            uvMinX,
            uvMinY,
            uvMaxX,
            uvMaxY
        };
    };

    const Vertex tl = transform(0.0f, 0.0f, u0, v0);
    const Vertex tr = transform(width, 0.0f, u1, v0);
    const Vertex br = transform(width, height, u1, v1);
    const Vertex bl = transform(0.0f, height, u0, v1);

    target.push_back(tl);
    target.push_back(tr);
    target.push_back(br);
    target.push_back(tl);
    target.push_back(br);
    target.push_back(bl);
}

void VulkanRenderer2D::EndFrame() {
    Flush();
}

void VulkanRenderer2D::Flush() {
    if (solidVertices_.empty() && texturedVertices_.empty()) {
        return;
    }
    UploadAndDraw();
    solidVertices_.clear();
    texturedVertices_.clear();
    textureDescriptorSet_ = VK_NULL_HANDLE;
}

void VulkanRenderer2D::UploadAndDraw() {
    FrameBuffer& buffer = vertexBuffers_[frameIndex_];
    std::vector<Vertex> vertices;
    vertices.reserve(solidVertices_.size() + texturedVertices_.size());
    vertices.insert(vertices.end(), solidVertices_.begin(), solidVertices_.end());
    vertices.insert(vertices.end(), texturedVertices_.begin(), texturedVertices_.end());

    if (uploadedVertices_ + vertices.size() > maxVertices_) {
        throw std::runtime_error("Vulkan 2D vertex buffer capacity exceeded");
    }

    const VkDeviceSize byteOffset = sizeof(Vertex) * static_cast<VkDeviceSize>(uploadedVertices_);
    const VkDeviceSize byteSize = sizeof(Vertex) * vertices.size();
    const VkDeviceSize mapSize = sizeof(Vertex) * static_cast<VkDeviceSize>(maxVertices_);

    void* mapped = nullptr;
    CheckVk(vkMapMemory(device_, buffer.memory, 0, mapSize, 0, &mapped),
            "mapping Vulkan 2D vertex buffer");
    auto* destination = static_cast<unsigned char*>(mapped) + byteOffset;
    std::memcpy(destination, vertices.data(), static_cast<std::size_t>(byteSize));
    vkUnmapMemory(device_, buffer.memory);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(viewportExtent_.width);
    viewport.height = static_cast<float>(viewportExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = scissor_;
    if (!scissorEnabled_) {
        scissor.offset = {0, 0};
        scissor.extent = viewportExtent_;
    }

    vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);

    const float pushConstants[] = {
        static_cast<float>(std::max(1u, logicalExtent_.width)),
        static_cast<float>(std::max(1u, logicalExtent_.height)),
        postProcess_.textureWidth,
        postProcess_.textureHeight,
        postProcess_.time,
        postProcess_.useFxaa,
        postProcess_.crtStrength,
        postProcess_.noiseAmount,
        postProcess_.brightness,
        postProcess_.contrast,
        postProcess_.sharpness,
        postProcess_.gamma,
        postProcess_.blackLevel,
        postProcess_.whiteLevel,
        postProcess_.screenBorder,
        postProcess_.scanlines,
        postProcess_.crtCurve,
        postProcess_.hdrEncoding,
        postProcess_.hdrPaperWhiteNits,
        postProcess_.hdrMaxNits,
        postProcess_.hdrHighlightStrength
    };
    static_assert(sizeof(pushConstants) <= 128, "Vulkan post-process push constants exceed the guaranteed limit");
    VkDeviceSize offsets[] = {byteOffset};
    vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &buffer.buffer, offsets);

    if (!solidVertices_.empty()) {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdPushConstants(commandBuffer_,
                           pipelineLayout_,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(pushConstants),
                           pushConstants);
        vkCmdDraw(commandBuffer_, static_cast<std::uint32_t>(solidVertices_.size()), 1, 0, 0);
    }

    if (!texturedVertices_.empty()) {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedPipeline_);
        vkCmdBindDescriptorSets(commandBuffer_,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                texturedPipelineLayout_,
                                0,
                                1,
                                &textureDescriptorSet_,
                                0,
                                nullptr);
        vkCmdPushConstants(commandBuffer_,
                           texturedPipelineLayout_,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(pushConstants),
                           pushConstants);
        vkCmdDraw(commandBuffer_,
                  static_cast<std::uint32_t>(texturedVertices_.size()),
                  1,
                  static_cast<std::uint32_t>(solidVertices_.size()),
                  0);
    }

    uploadedVertices_ += static_cast<std::uint32_t>(vertices.size());
}

} // namespace wbwwb::vulkan
