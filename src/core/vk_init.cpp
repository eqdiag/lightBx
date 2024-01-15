#include "vk_init.h"
#include "vk_log.h"

#include <iostream>

VkCommandPoolCreateInfo vk_init::commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.queueFamilyIndex = queueFamilyIndex;
	create_info.flags = flags;

	return create_info;
}

VkCommandBufferAllocateInfo vk_init::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	
	alloc_info.commandPool = pool;
	alloc_info.commandBufferCount = count;
	alloc_info.level = level;

	return alloc_info;
}

VkFenceCreateInfo vk_init::fenceCreateInfo(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.flags = flags;

	return create_info;
}

VkSemaphoreCreateInfo vk_init::semaphoreCreateInfo(VkSemaphoreCreateFlags flags)
{
	VkSemaphoreCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.flags = flags;

	return create_info;
}

VkPipelineShaderStageCreateInfo vk_init::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
	VkPipelineShaderStageCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	create_info.pNext = nullptr;
	
	//Shader stage
	create_info.stage = stage;
	create_info.module = shaderModule;
	//Entry point
	create_info.pName = "main";

	return create_info;
}

VkPipelineVertexInputStateCreateInfo vk_init::pipelineVertexInputStateCreateInfo()
{
	VkPipelineVertexInputStateCreateInfo create_info{};

	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	create_info.pNext = nullptr;

	/* Bindings */
	create_info.vertexBindingDescriptionCount = 0;
	create_info.pVertexBindingDescriptions = nullptr;

	/* Attributes */
	create_info.vertexAttributeDescriptionCount = 0;
	create_info.pVertexAttributeDescriptions = nullptr;

	return create_info;
}

VkPipelineInputAssemblyStateCreateInfo vk_init::pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.topology = topology;
	create_info.primitiveRestartEnable = VK_FALSE;

	return create_info;
}

VkPipelineRasterizationStateCreateInfo vk_init::pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode)
{
	VkPipelineRasterizationStateCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.depthClampEnable = VK_FALSE;
	create_info.rasterizerDiscardEnable = VK_FALSE;

	create_info.polygonMode = polygonMode;
	create_info.lineWidth = 1.0f;
	//no backface cull
	create_info.cullMode = VK_CULL_MODE_NONE;
	create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	//no depth bias
	create_info.depthBiasEnable = VK_FALSE;
	create_info.depthBiasConstantFactor = 0.0f;
	create_info.depthBiasClamp = 0.0f;
	create_info.depthBiasSlopeFactor = 0.0f;

	return create_info;
}

VkPipelineColorBlendAttachmentState vk_init::pipelineColorBlendAttachmentState()
{
	VkPipelineColorBlendAttachmentState state{};
	state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	state.blendEnable = VK_FALSE;
	return state;
}

VkPipelineMultisampleStateCreateInfo vk_init::pipelineMultisampleStateCreateInfo()
{
	VkPipelineMultisampleStateCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.sampleShadingEnable = VK_FALSE;
	create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	create_info.minSampleShading = 1.0f;
	create_info.pSampleMask = nullptr;
	create_info.alphaToCoverageEnable = VK_FALSE;
	create_info.alphaToOneEnable = VK_FALSE;
	return create_info;
}

VkPipelineLayoutCreateInfo vk_init::pipelineLayoutCreateInfo()
{
	VkPipelineLayoutCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.flags = 0;

	/* Push Constants */
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = nullptr;

	/* Layouts */
	create_info.setLayoutCount = 0;
	create_info.pSetLayouts = nullptr;

	return create_info;
}

VkPipeline vk_init::PipelineBuilder::build(VkDevice device, VkRenderPass renderPass)
{

	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = nullptr;

	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &_viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &_scissor;

	//dummy blending
	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.pNext = nullptr;

	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &_colorBlendAttachment;


	VkGraphicsPipelineCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.stageCount = _shaderStages.size();
	create_info.pStages = _shaderStages.data();
	create_info.pVertexInputState = &_vertexInputInfo;
	create_info.pInputAssemblyState = &_inputAssembly;
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &_rasterizer;
	create_info.pMultisampleState = &_multisampling;
	create_info.pColorBlendState = &color_blending;
	create_info.layout = _layout;
	create_info.renderPass = renderPass;
	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline{};

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline) != VK_SUCCESS) {
		std::cout << "Failed to create pipeline!" <<  std::endl;
		return VK_NULL_HANDLE; 
	}
	else {
		return pipeline;
	}
}
