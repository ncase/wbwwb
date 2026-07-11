#include "VulkanPipeline.h"

#include "VulkanErrors.h"
#include "VulkanShaderModule.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace wbwwb::vulkan {

namespace {

struct Solid2DVertexLayout {
    float position[2];
    float uv[2];
    float color[4];
    float uvMin[2];
    float uvMax[2];
};

std::vector<std::uint32_t> ReadSpirvWords(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open Vulkan shader: " + path.string());
    }

    const std::streamsize byteSize = file.tellg();
    if (byteSize <= 0 || (byteSize % static_cast<std::streamsize>(sizeof(std::uint32_t))) != 0) {
        throw std::runtime_error("Vulkan shader is empty or not word-aligned: " + path.string());
    }

    std::vector<std::uint32_t> words(static_cast<std::size_t>(byteSize) / sizeof(std::uint32_t));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(words.data()), byteSize)) {
        throw std::runtime_error("Failed to read Vulkan shader: " + path.string());
    }
    return words;
}

} // namespace

void VulkanPipeline::CreateSolid2D(VkDevice device,
                                   VkRenderPass renderPass,
                                   VkExtent2D viewportExtent,
                                   const std::filesystem::path& shaderDirectory,
                                   const std::filesystem::path& texturedFragmentShaderPath,
                                   VkSampleCountFlagBits samples) {
    device_ = device;

    VulkanShaderModule vertexShader;
    VulkanShaderModule fragmentShader;
    vertexShader.Create(device_, ReadSpirvWords(shaderDirectory / "solid2d.vert.spv"));
    fragmentShader.Create(device_, ReadSpirvWords(shaderDirectory / "solid2d.frag.spv"));

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShader.Get();
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShader.Get();
    shaderStages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Solid2DVertexLayout);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> attributes{};
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[0].offset = offsetof(Solid2DVertexLayout, position);
    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[1].offset = offsetof(Solid2DVertexLayout, uv);
    attributes[2].binding = 0;
    attributes[2].location = 2;
    attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[2].offset = offsetof(Solid2DVertexLayout, color);
    attributes[3].binding = 0;
    attributes[3].location = 3;
    attributes[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[3].offset = offsetof(Solid2DVertexLayout, uvMin);
    attributes[4].binding = 0;
    attributes[4].location = 4;
    attributes[4].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[4].offset = offsetof(Solid2DVertexLayout, uvMax);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &binding;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(viewportExtent.width);
    viewport.height = static_cast<float>(viewportExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = viewportExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = samples;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange pushConstants{};
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstants.offset = 0;
    pushConstants.size = sizeof(float) * 21;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstants;
    CheckVk(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &layout_),
            "creating Vulkan 2D pipeline layout");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<std::uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    CheckVk(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_),
            "creating Vulkan 2D graphics pipeline");

    fragmentShader.Destroy();
    vertexShader.Destroy();

    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = 1;
    descriptorSetLayoutInfo.pBindings = &textureBinding;
    CheckVk(vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutInfo, nullptr, &textureDescriptorSetLayout_),
            "creating Vulkan texture descriptor set layout");

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 512;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 512;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    CheckVk(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &textureDescriptorPool_),
            "creating Vulkan texture descriptor pool");

    VkPipelineLayoutCreateInfo texturedLayoutInfo{};
    texturedLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    texturedLayoutInfo.setLayoutCount = 1;
    texturedLayoutInfo.pSetLayouts = &textureDescriptorSetLayout_;
    texturedLayoutInfo.pushConstantRangeCount = 1;
    texturedLayoutInfo.pPushConstantRanges = &pushConstants;
    CheckVk(vkCreatePipelineLayout(device_, &texturedLayoutInfo, nullptr, &texturedLayout_),
            "creating Vulkan textured 2D pipeline layout");

    VulkanShaderModule texturedVertexShader;
    VulkanShaderModule texturedFragmentShader;
    texturedVertexShader.Create(device_, ReadSpirvWords(shaderDirectory / "textured2d.vert.spv"));
    texturedFragmentShader.Create(device_, ReadSpirvWords(shaderDirectory / texturedFragmentShaderPath));
    shaderStages[0].module = texturedVertexShader.Get();
    shaderStages[1].module = texturedFragmentShader.Get();
    pipelineInfo.layout = texturedLayout_;

    CheckVk(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &texturedPipeline_),
            "creating Vulkan textured 2D graphics pipeline");

    texturedFragmentShader.Destroy();
    texturedVertexShader.Destroy();
}

VkDescriptorSet VulkanPipeline::CreateTextureDescriptorSet(VkImageView imageView, VkSampler sampler) {
    if (textureDescriptorPool_ == VK_NULL_HANDLE || textureDescriptorSetLayout_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Texture descriptor requested before Vulkan textured pipeline creation");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = textureDescriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureDescriptorSetLayout_;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    CheckVk(vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet),
            "allocating Vulkan texture descriptor set");

    UpdateTextureDescriptorSet(descriptorSet, imageView, sampler);
    return descriptorSet;
}

void VulkanPipeline::UpdateTextureDescriptorSet(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler) {
    if (descriptorSet == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot update an incomplete Vulkan texture descriptor");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

}

void VulkanPipeline::Destroy() {
    if (texturedPipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, texturedPipeline_, nullptr);
        texturedPipeline_ = VK_NULL_HANDLE;
    }
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (texturedLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, texturedLayout_, nullptr);
        texturedLayout_ = VK_NULL_HANDLE;
    }
    if (layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, layout_, nullptr);
        layout_ = VK_NULL_HANDLE;
    }
    if (textureDescriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, textureDescriptorPool_, nullptr);
        textureDescriptorPool_ = VK_NULL_HANDLE;
    }
    if (textureDescriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, textureDescriptorSetLayout_, nullptr);
        textureDescriptorSetLayout_ = VK_NULL_HANDLE;
    }
    device_ = VK_NULL_HANDLE;
}

} // namespace wbwwb::vulkan
