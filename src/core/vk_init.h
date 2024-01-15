#pragma once

#include "vk_types.h"

#include <vector>

namespace vk_init {

	/* Commands */
	VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	
	VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	/* Sync */

	VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

	/* Pipelines */

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology);

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode);

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState();

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
		VkPipelineMultisampleStateCreateInfo _multisampling{};

		//Layout
		VkPipelineLayout _layout{};
	};


	/* Desciptors */

	/* Buffers */

	/* Images */

}