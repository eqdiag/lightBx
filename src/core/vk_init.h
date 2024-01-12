#pragma once

#include "vk_types.h"
#include <vector>


namespace vk_init{

    /* Graphics Pipeline */

    class PipelineBuilder{
        public:

            VkPipeline buildPipeline(VkDevice device,VkRenderPass pass);

            //Programmable shader stages
            std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

            //Input format for vertex data
            VkPipelineVertexInputStateCreateInfo _vertexInputInfo;

            //Primitive construction from vertex data
            VkPipelineInputAssemblyStateCreateInfo _inputAssembly;

            /* Dynamic state */

            //Viewport
            VkViewport _viewport;

            //Scissor
            VkRect2D _scissor;

            //Depth biasing,culling
            VkPipelineRasterizationStateCreateInfo _rasterizer;

            //Depth testing
            VkPipelineDepthStencilStateCreateInfo _depthStencil;

            //Color blending
            VkPipelineColorBlendAttachmentState _colorBlendAttachment;

            //Multisampling
            VkPipelineMultisampleStateCreateInfo _multisampling;

            //Layout = expected input resources (push consts,bindings)
            VkPipelineLayout _pipelineLayout;

    };

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,VkShaderModule module);
    
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology);

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode);

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo();

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(bool depthTest,bool depthWrite,VkCompareOp compareOp);

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState();

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();

    /* Commands */

    VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamily);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool cmdPool, uint32_t numBuffers);

    VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

    VkSubmitInfo submitInfo(VkCommandBuffer* cmd);

    /* Images */

    VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

    VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    VkSamplerCreateInfo samplerCreateInfo(VkFilter filters, VkSamplerAddressMode addressMode);

    VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);


    /* Buffers */

    void createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocator allocator,AllocatedBuffer* allocatedBuffer);

    size_t padBufferSize(size_t minAlignmentSize, size_t originalSize);

   /* Descriptor Sets */

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);

    VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet descSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
}