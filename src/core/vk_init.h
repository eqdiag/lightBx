#pragma once

#include "vk_types.h"

#include <vector>

namespace vk_init {

	/* Commands */
	VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	
	VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags);

	VkSubmitInfo submitInfo(VkCommandBuffer* cmd);

	/* Sync */

	VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

	/* Pipelines */

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology);

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode);

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState();

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp);

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo();

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();
	
	class PipelineBuilder {
	public:
		VkPipeline build(VkDevice device, VkRenderPass renderPass);

		//Vertex stages
		std::vector<VkPipelineShaderStageCreateInfo> _shaderStages{};
		VkPipelineVertexInputStateCreateInfo _vertexInputInfo{};
		VkPipelineInputAssemblyStateCreateInfo _inputAssembly{};

		VkViewport _viewport{};
		VkRect2D _scissor{};

		//Fragment stages
		VkPipelineRasterizationStateCreateInfo _rasterizer{};
		VkPipelineColorBlendAttachmentState _colorBlendAttachment{};
		VkPipelineDepthStencilStateCreateInfo _depthStencil;
		VkPipelineMultisampleStateCreateInfo _multisampling{};

		//Layout
		VkPipelineLayout _layout{};
	};


	/* Desciptors */

	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(uint32_t numBindings, const VkDescriptorSetLayoutBinding* bindings);

	VkDescriptorBufferInfo descriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size);

	VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType type, VkDescriptorBufferInfo* bufferInfo);

	VkWriteDescriptorSet writeDescriptorImage(VkDescriptorSet dstSet,uint32_t dstBinding,VkDescriptorType type,VkDescriptorImageInfo* imageInfo);

	/* Samplers */

	VkSamplerCreateInfo samplerCreateInfo(VkFilter filters, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	/* Buffers */

	/* Images */

	VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

	VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags flags);

}