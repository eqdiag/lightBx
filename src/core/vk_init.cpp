#include "vk_init.h"
#include "vk_log.h"

VkPipeline vk_init::PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass)
{
    
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = nullptr;

    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &_viewport;

    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &_scissor;

    //Dummy blending
    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.pNext = nullptr;

    color_blend.logicOpEnable = VK_FALSE;
    color_blend.logicOp = VK_LOGIC_OP_COPY;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &_colorBlendAttachment;


    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.stageCount = _shaderStages.size();
    create_info.pStages = _shaderStages.data();
    create_info.pVertexInputState = &_vertexInputInfo;
    create_info.pInputAssemblyState = &_inputAssembly;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &_rasterizer;
    create_info.pDepthStencilState = &_depthStencil;
    create_info.pMultisampleState = &_multisampling;
    create_info.pColorBlendState = &color_blend;
    create_info.layout = _pipelineLayout;
    create_info.renderPass = pass;
    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    
    VK_CHECK(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&create_info,nullptr,&pipeline));
    
    return pipeline;
}

VkPipelineShaderStageCreateInfo vk_init::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module)
{
    VkPipelineShaderStageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    create_info.pNext = nullptr;

    //Shader stage
    create_info.stage = stage;
    //Actual module
    create_info.module = module;

    //Shader entry point
    create_info.pName = "main";

    return create_info;
}

VkPipelineVertexInputStateCreateInfo vk_init::pipelineVertexInputStateCreateInfo()
{
    VkPipelineVertexInputStateCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    create_info.pNext = nullptr;

    //No bindings
    create_info.vertexBindingDescriptionCount = 0;
    //No attributes
    create_info.vertexAttributeDescriptionCount = 0;
    return create_info;
}

VkPipelineInputAssemblyStateCreateInfo vk_init::pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.topology = topology;
    //Can re-use vertex data
    create_info.primitiveRestartEnable = VK_FALSE;
    return create_info;
}

VkPipelineRasterizationStateCreateInfo vk_init::pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode)
{
    VkPipelineRasterizationStateCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.depthClampEnable = VK_FALSE;
    //Discard primitives before rasterization
    create_info.rasterizerDiscardEnable = VK_FALSE;

    create_info.polygonMode = polygonMode;
    create_info.lineWidth = 1.0f;

    //Face culling
    create_info.cullMode = VK_CULL_MODE_NONE;
    create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;

    //Depth bias
    create_info.depthBiasEnable = VK_FALSE;
    create_info.depthBiasConstantFactor = 0.0f;
    create_info.depthBiasClamp = 0.0f;
    create_info.depthBiasSlopeFactor = 0.0f;

    return create_info;
}

VkPipelineMultisampleStateCreateInfo vk_init::pipelineMultisampleStateCreateInfo()
{
    VkPipelineMultisampleStateCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.sampleShadingEnable = VK_FALSE;
    //1 sample per pixel
    create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    create_info.minSampleShading = 1.0f;
    create_info.pSampleMask = nullptr;
    create_info.alphaToCoverageEnable = VK_FALSE;
    create_info.alphaToOneEnable = VK_FALSE;

    return create_info;
}

VkPipelineDepthStencilStateCreateInfo vk_init::pipelineDepthStencilStateCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp)
{
    VkPipelineDepthStencilStateCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    create_info.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
    create_info.stencilTestEnable = VK_FALSE;


    create_info.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
    create_info.depthBoundsTestEnable = VK_FALSE;
    create_info.minDepthBounds = 0.0f; // Optional
    create_info.maxDepthBounds = 1.0f; // Optional

    return create_info;
}

VkPipelineColorBlendAttachmentState vk_init::pipelineColorBlendAttachmentState()
{
    VkPipelineColorBlendAttachmentState attachment{};
    
    attachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;

    attachment.blendEnable = VK_FALSE;

    return attachment;
}

VkPipelineLayoutCreateInfo vk_init::pipelineLayoutCreateInfo()
{
    VkPipelineLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.flags = 0;

    //No layout
    create_info.setLayoutCount = 0;
    create_info.pSetLayouts = nullptr;

    //No push constants
    create_info.pushConstantRangeCount = 0;
    create_info.pPushConstantRanges = nullptr;

    return create_info;
}

VkCommandPoolCreateInfo vk_init::commandPoolCreateInfo(uint32_t queueFamily)
{
    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.pNext = nullptr;

    //Pool that has commands compatible with queues in this family
    create_info.queueFamilyIndex = queueFamily;
    //Allows commands from this pool to be reset individually
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    return create_info;
}

VkCommandBufferAllocateInfo vk_init::commandBufferAllocateInfo(VkCommandPool cmdPool, uint32_t numBuffers)
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    //Allocate a buffer from command pool
    alloc_info.commandPool = cmdPool;
    //Allocate potentially a batch at a time
    alloc_info.commandBufferCount = numBuffers;
    //Primary = sent directly to queue, secondary are not (ADVANCED)
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    return alloc_info;
}

VkCommandBufferBeginInfo vk_init::commandBufferBeginInfo(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;

    //Used for secondary buffers
    begin_info.pInheritanceInfo = nullptr;
    //Let driver know we're only submitting this buffer once
    begin_info.flags = flags;

    return begin_info;
}

VkSubmitInfo vk_init::submitInfo(VkCommandBuffer* cmd)
{
    VkSubmitInfo submit_info{};

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;

    submit_info.pWaitDstStageMask = nullptr;

    //Wait for swapchain to have img ready
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;

    //Signal render done flag when done
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    //Submit single recorded command buffer
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd;

    return submit_info;
}

VkImageCreateInfo vk_init::imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
    VkImageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.imageType = VK_IMAGE_TYPE_2D;

    create_info.format = format;
    create_info.extent = extent;

    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usageFlags;

    return create_info;
}

VkImageViewCreateInfo vk_init::imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.image = image;
    create_info.format = format;

    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.layerCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.aspectMask = aspectFlags;

    return create_info;
}

VkSamplerCreateInfo vk_init::samplerCreateInfo(VkFilter filters, VkSamplerAddressMode addressMode)
{
    VkSamplerCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.minFilter = filters;
    create_info.magFilter = filters;
    create_info.addressModeU = addressMode;
    create_info.addressModeV = addressMode;
    create_info.addressModeW = addressMode;

    return create_info;
}

VkWriteDescriptorSet vk_init::writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding)
{
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = imageInfo;

    return write;
}

void vk_init::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,VmaAllocator allocator, AllocatedBuffer* allocatedBuffer)
{
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.size = allocSize;
    //Using as vertex buffer
    create_info.usage = usage;

    VmaAllocationCreateInfo alloc_info{};
    //Writeable by cpu,readable by gpu
    alloc_info.usage = memoryUsage;

    //Reserve actual memory for buffer
    VK_CHECK(vmaCreateBuffer(allocator, &create_info, &alloc_info, &allocatedBuffer->_buffer, &allocatedBuffer->_allocation, nullptr));
}

size_t vk_init::padBufferSize(size_t minAlignmentSize, size_t originalSize)
{
    size_t minSize = minAlignmentSize;
    size_t alignedSize = originalSize;
    if (minSize > 0) {
        alignedSize = (alignedSize + minSize - 1) & ~(minSize - 1);
    }
    return alignedSize;
}

VkDescriptorSetLayoutBinding vk_init::descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding)
{
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = binding;
    //Would put multiple here if it was an array say
    layout_binding.descriptorCount = 1;
    //Let driver know there will be a uniform buffer here
    layout_binding.descriptorType = type;
    //Just use in vertex shader
    layout_binding.stageFlags = stageFlags;

    return layout_binding;
}

VkWriteDescriptorSet vk_init::writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet descSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding)
{
    VkWriteDescriptorSet write_set{};

    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;

    //Write into binding "binding"
    write_set.dstBinding = binding;
    //Of this descriptor set "descSet"
    write_set.dstSet = descSet;
    //Just write 1 descriptor
    write_set.descriptorCount = 1;

    write_set.descriptorType = type;
    write_set.pBufferInfo = bufferInfo;

    return write_set;
}
