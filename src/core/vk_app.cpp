#include "vk_app.h"
#include "vk_log.h"
#include "vk_init.h"
#include "vk_util.h"
#include "vk_io.h"
#include "settings.h"
#include "VkBootstrap.h"

#include "primitives/mesh.h"
#include "primitives/camera.h"
#include "primitives/shapes.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void VkApp::init()
{
	initWindow();

	initVulkan();

	initSwapchain();

	initCommands();

	initSync();

	initSamplers();

	initRenderPasses();

	initFrameBuffers();

	initBuffers();

	initImages();

	initDescriptors();

	initPipelines();

	initImgui();

	_init = true;

}

void VkApp::run()
{
	while (!glfwWindowShouldClose(_window)) {

		glfwPollEvents();

		//TODO: imgui glfw callbacks?
	

		if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS) {
			_mainCamera.translate(-TRANSLATE_SPEED, 0.0, 0.0);
		}else if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS) {
			_mainCamera.translate(TRANSLATE_SPEED, 0.0, 0.0);
		}else if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS) {
			_mainCamera.translate(0.0, 0.0, -TRANSLATE_SPEED);
		}else if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS) {
			_mainCamera.translate(0.0, 0.0, TRANSLATE_SPEED);
		}


		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();


		drawUI();

		draw();

	}
}

void VkApp::draw()
{
	//Get current frame data
	auto& frame = getFrame();
	uint32_t frameIdx = _frameNum % NUM_FRAMES;


	VK_CHECK(vkWaitForFences(_device, 1, &frame._renderDoneFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &frame._renderDoneFence));

	/* Update frame resources */

	//Update camera info
	math::Mat4 view = _mainCamera.getViewMatrix();
	math::Mat4 proj = math::Mat4::perspectiveProjection(70.0 * (3.14 / 180.0), (float)_windowSize.width / (float)_windowSize.height, 0.1f, 200.0f);
	proj[1][1] *= -1;
	GPUCameraData gpu_data{};
	gpu_data.view_proj = proj * view;
	auto eye = _mainCamera.getEye();
	gpu_data.eye = math::Vec4{ eye.x(),eye.y(),eye.z(),0.0};

	//Copy to gpu
	uint32_t camOffsetSize = vk_util::padBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(GPUCameraData));
	char* data;
	vmaMapMemory(_allocator, _cameraBuffer._allocation, (void**)&data);

	data += camOffsetSize * frameIdx;
	memcpy(data, (void*)&gpu_data, camOffsetSize);

	vmaUnmapMemory(_allocator, _cameraBuffer._allocation);


	//Update light data
	uint32_t lightOffsetSize = vk_util::padBufferSize(_gpuProperties.limits.minStorageBufferOffsetAlignment, sizeof(LightEntity)) * NUM_LIGHTS;
	size_t light_buffer_size = vk_util::padBufferSize(_gpuProperties.limits.minStorageBufferOffsetAlignment, sizeof(LightEntity));
	light_buffer_size *= NUM_LIGHTS * NUM_FRAMES;


	//Fill buffer with data
	void* light_data;
	vmaMapMemory(_allocator, _lightBuffer._allocation, &light_data);

	LightEntity* light = (LightEntity*)light_data;
	for (uint32_t i = 0; i < NUM_LIGHTS; i++) {
		light[i + frameIdx * NUM_LIGHTS].position = math::Vec4{ 4.0f * i, 6.0f, 2.0f * i + 4.0f * static_cast<float>(sin(glfwGetTime())),0.0};
		//light[0].position = math::Vec4{ 0, 6.0f + 4.0f * static_cast<float>(sin(glfwGetTime())), 0,0.0 };

	}
	vmaUnmapMemory(_allocator, _lightBuffer._allocation);


	uint32_t nextImgIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, frame._imgReadyFlag, VK_NULL_HANDLE, &nextImgIndex));

	VkCommandBuffer cmd = frame._commandBuffer;

	//Reset command buffer
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo begin_info = vk_init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	ImGui::Render();

	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	VkClearValue clearValue;
	//float flash = abs(sin(_frameNum / 120.0f));
	clearValue.color = { {0.2,0.2,0.2,1.0f} };

	VkClearValue clearDepth;
	clearDepth.depthStencil.depth = 1.0f;

	VkClearValue clearValues[] = { clearValue,clearDepth };

	VkRenderPassBeginInfo pass_begin_info{};
	pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass_begin_info.pNext = nullptr;

	pass_begin_info.renderPass = _renderPass;
	pass_begin_info.renderArea.offset.x = 0;
	pass_begin_info.renderArea.offset.y = 0;
	pass_begin_info.renderArea.extent = _windowSize;
	pass_begin_info.framebuffer = _frameBuffers[nextImgIndex];

	pass_begin_info.clearValueCount = 2;
	pass_begin_info.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	/* Draw lights */

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _lightPipeline);

	//Bind vertex buffer
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &_vertexBuffer._buffer, &offset);

	//Bind index buffer
	//vkCmdBindIndexBuffer(cmd, _indexBuffer._buffer, offset, VK_INDEX_TYPE_UINT32);

	//View/proj
	//uint32_t dynamicOffset =

	uint32_t dynamicOffsets[] = { camOffsetSize * frameIdx,lightOffsetSize*frameIdx };
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _lightPipelineLayout, 0, 1, &_lightDescriptorSet, 2, dynamicOffsets);
	
	//Instanced draw
	vkCmdDraw(cmd, _vertices.size(), NUM_LIGHTS, 0, 0);
	//vkCmdDrawIndexed(cmd, static_cast<uint32_t>(_indices.size()), NUM_LIGHTS, 0, 0, 0);

	/* Draw Objects */

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _objectPipeline);

	//Bind vertex buffer
	//VkDeviceSize offset = 0;
	offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &_vertexBuffer._buffer, &offset);

	//Bind index buffer
	//vkCmdBindIndexBuffer(cmd, _indexBuffer._buffer, offset, VK_INDEX_TYPE_UINT32);

	//View/proj
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _objectPipelineLayout, 0, 1, &_objectDescriptorSet, 2, dynamicOffsets);

	//Instanced draw
	//vkCmdDrawIndexed(cmd, static_cast<uint32_t>(_indices.size()), NUM_OBJECTS, 0, 0, 0);
	vkCmdDraw(cmd, _vertices.size(), NUM_OBJECTS, 0, 0);


	//Imgui draw commands
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	
	vkCmdEndRenderPass(cmd);


	VK_CHECK(vkEndCommandBuffer(cmd));

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	/*
	TODO: Get multi-viewport workingg
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}*/

	//Submit recorded buffer to graphics queue
	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &frame._imgReadyFlag;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &frame._renderDoneFlag;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, frame._renderDoneFence));

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;

	present_info.swapchainCount = 1;
	present_info.pSwapchains = &_swapchain;

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &frame._renderDoneFlag;

	present_info.pImageIndices = &nextImgIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &present_info));

	_frameNum++;
}

void VkApp::cleanup()
{
	if (_init) {

		for (uint32_t i = 0; i < NUM_FRAMES; i++) {
			VK_CHECK(vkWaitForFences(_device, 1,&_frames[i]._renderDoneFence, true, 1000000000));
		}

		destroyImgui();

		destroyPipelines();

		destroyDescriptors();

		destroyBuffers();

		destroyImages();

		destroySamplers();

		destroySync();

		destroyFrameBuffers();

		destroyRenderPasses();

		destroyCommands();

		destroySwapchain();

		destroyVulkan();

		destroyWindow();
	}
}

void VkApp::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	_window = glfwCreateWindow(_windowSize.width, _windowSize.height, "lightBx", nullptr, nullptr);

	glfwSetCursorPosCallback(_window, _cursorMotionCallback);

	glfwSetMouseButtonCallback(_window, _mouseButtonCallback);

	glfwSetWindowUserPointer(_window, (void*)this);

	int width,height;
	glfwGetFramebufferSize(_window,&width,&height);

	_windowSize.width = width;
	_windowSize.height = height;
}

void VkApp::initVulkan()
{
	//Create instance
	vkb::InstanceBuilder instance_builder{};

	auto instance_result = instance_builder
		.set_app_name("lightBx")
		.request_validation_layers(true)
		.require_api_version(1, 3, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance instance = instance_result.value();

	_instance = instance.instance;
	_debugMessenger = instance.debug_messenger;

	//Create surface
	VK_CHECK(glfwCreateWindowSurface(_instance, _window, nullptr, &_surface));

	//Create physical device
	vkb::PhysicalDeviceSelector selector{instance};

	vkb::PhysicalDevice gpu = selector
		.set_minimum_version(1, 1)
		.set_surface(_surface)
		.select()
		.value();

	_gpu = gpu.physical_device;


	//Create device
	vkb::DeviceBuilder device_builder{gpu};
	VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
	shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shader_draw_parameters_features.pNext = nullptr;
	shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;

	vkb::Device device = device_builder
		.add_pNext(&shader_draw_parameters_features)
		.build()
		.value();

	_device = device.device;

	_gpuProperties = device.physical_device.properties;

	_graphicsFamilyQueueIndex = device.get_queue_index(vkb::QueueType::graphics).value();
	_graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();

	//Create vma allocator

	VmaAllocatorCreateInfo create_info{};
	create_info.instance = _instance;
	create_info.physicalDevice = _gpu;
	create_info.device = _device;
	VK_CHECK(vmaCreateAllocator(&create_info, &_allocator));
}

void VkApp::initSwapchain()
{
	vkb::SwapchainBuilder swapchain_builder{_gpu, _device, _surface};

	VkSurfaceFormatKHR surface_format{};
	surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
	surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	vkb::Swapchain swapchain = swapchain_builder
		.use_default_format_selection()
		.set_desired_format(surface_format)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_windowSize.width,_windowSize.height)
		.build()
		.value();

	_swapchain = swapchain.swapchain;
	_swapchainFormat = swapchain.image_format;
	_swapchainImages = swapchain.get_images().value();
	_swapchainImageViews = swapchain.get_image_views().value();

	VkExtent3D depthExtent{
		_windowSize.width,
		_windowSize.height,
		1
	};

	_depthFormat = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo img_create_info = vk_init::imageCreateInfo(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthExtent);

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vmaCreateImage(_allocator, &img_create_info, &alloc_info, &_depthImage._image, &_depthImage._allocation, nullptr));

	VkImageViewCreateInfo view_create_info = vk_init::imageViewCreateInfo(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(_device, &view_create_info, nullptr, &_depthImageView));
}

void VkApp::initCommands()
{
	//Enable buffer resetting
	VkCommandPoolCreateInfo pool_create_info = vk_init::commandPoolCreateInfo(_graphicsFamilyQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	//Frame objects
	for (uint32_t i = 0; i < NUM_FRAMES; i++) {

		VK_CHECK(vkCreateCommandPool(_device, &pool_create_info, nullptr, &_frames[i]._commandPool));

		//Allocate 1 primary command buffer
		VkCommandBufferAllocateInfo buffer_alloc_info = vk_init::commandBufferAllocateInfo(_frames[i]._commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VK_CHECK(vkAllocateCommandBuffers(_device, &buffer_alloc_info, &_frames[i]._commandBuffer));
	}

	//Upload context objects
	VK_CHECK(vkCreateCommandPool(_device, &pool_create_info, nullptr, &_uploadContext._commandPool));
	VkCommandBufferAllocateInfo buffer_alloc_info = vk_init::commandBufferAllocateInfo(_uploadContext._commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VK_CHECK(vkAllocateCommandBuffers(_device, &buffer_alloc_info, &_uploadContext._commandBuffer));
}


void VkApp::initRenderPasses()
{

	/* Attachments */
	VkAttachmentDescription color_attachment{};
	color_attachment.format = _swapchainFormat;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = _depthFormat;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription attachments[] = { color_attachment,depth_attachment };

	/* Subpasses */
	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref{};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	/* Dependencies */
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depth_dependency{};
	depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depth_dependency.dstSubpass = 0;
	depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.srcAccessMask = 0;
	depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


	VkSubpassDependency dependencies[] = {dependency,depth_dependency};

	/* Render pass */

	VkRenderPassCreateInfo pass_create_info{};
	pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	pass_create_info.pNext = nullptr;

	pass_create_info.attachmentCount = 2;
	pass_create_info.pAttachments = &attachments[0];

	pass_create_info.subpassCount = 1;
	pass_create_info.pSubpasses = &subpass;

	pass_create_info.dependencyCount = 2;
	pass_create_info.pDependencies = dependencies;

	VK_CHECK(vkCreateRenderPass(_device,&pass_create_info,nullptr,&_renderPass));
}

void VkApp::initFrameBuffers()
{
	VkFramebufferCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.renderPass = _renderPass;
	create_info.attachmentCount = 1;
	create_info.width = _windowSize.width;
	create_info.height = _windowSize.height;
	create_info.layers = 1;

	uint32_t numImages = _swapchainImages.size();
	_frameBuffers.resize(numImages);

	for (uint32_t i = 0; i < numImages; i++) {

		VkImageView attachments[] = { _swapchainImageViews[i],_depthImageView };

		create_info.attachmentCount = 2;
		create_info.pAttachments = attachments;

		VK_CHECK(vkCreateFramebuffer(_device,&create_info,nullptr,&_frameBuffers[i]));
	}

}

void VkApp::initSync()
{
	//Start fence in signaled state
	VkFenceCreateInfo fence_create_info = vk_init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

	VkSemaphoreCreateInfo semaphore_create_info = vk_init::semaphoreCreateInfo();

	//Frame sync objects
	for (uint32_t i = 0; i < NUM_FRAMES; i++) {

		VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[i]._renderDoneFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._imgReadyFlag));
		VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._renderDoneFlag));
	}

	//Upload sync objects
	//Don't start fence in signaled state
	fence_create_info.flags = 0;
	VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_uploadContext._uploadDoneFence));
}

void VkApp::initSamplers()
{
	VkSamplerCreateInfo info = vk_init::samplerCreateInfo(VK_FILTER_NEAREST);

	VK_CHECK(vkCreateSampler(_device, &info, nullptr, &_blockySampler));
}

void VkApp::initPipelines()
{

	/* Light Pipeline Creation */
	VkShaderModule vertexShader{};
	VkShaderModule fragShader{};

	std::string vertexShaderPath = SHADER_DIR + std::string{"light.vert.spv"};
	std::string fragShaderPath = SHADER_DIR + std::string{"light.frag.spv"};


	if (!vk_io::loadShaderModule(_device, vertexShaderPath.c_str(), &vertexShader)) {
		std::cerr << "Couldn't load vertex shader: " << vertexShaderPath << std::endl;
	}
	else {
		std::cout << "Loaded vertex shader: " << vertexShaderPath << std::endl;
	}

	if (!vk_io::loadShaderModule(_device, fragShaderPath.c_str(), &fragShader)) {
		std::cerr << "Couldn't load fragment shader: " << fragShaderPath << std::endl;
	}
	else {
		std::cout << "Loaded fragment shader: " << fragShaderPath << std::endl;
	}

	vk_init::PipelineBuilder pipeline_builder{};


	pipeline_builder._shaderStages.emplace_back(
		vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader)
	);

	pipeline_builder._shaderStages.emplace_back(
		vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
	);

	/* Pipeline layout */

	VkPipelineLayoutCreateInfo pipeline_layout_info = vk_init::pipelineLayoutCreateInfo();

	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &_lightDescriptorSetLayout;

	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_lightPipelineLayout));

	/* Vertex input */

	VkPipelineVertexInputStateCreateInfo vertex_input_info = vk_init::pipelineVertexInputStateCreateInfo();

	auto description = vk_primitives::mesh::Vertex_F3_F3_F2::getVertexInputDescription();

	vertex_input_info.vertexBindingDescriptionCount = description._bindingDescriptions.size();
	vertex_input_info.vertexAttributeDescriptionCount = description._attributeDescriptions.size();

	vertex_input_info.pVertexBindingDescriptions = description._bindingDescriptions.data();
	vertex_input_info.pVertexAttributeDescriptions = description._attributeDescriptions.data();

	pipeline_builder._vertexInputInfo = vertex_input_info;

	/* Input Assembly */

	pipeline_builder._inputAssembly = vk_init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	pipeline_builder._viewport.x = 0.0f;
	pipeline_builder._viewport.y = 0.0f;
	pipeline_builder._viewport.width = (float)_windowSize.width;
	pipeline_builder._viewport.height = (float)_windowSize.height;
	pipeline_builder._viewport.minDepth = 0.0f;
	pipeline_builder._viewport.maxDepth = 1.0f;

	pipeline_builder._scissor.offset = { 0, 0 };
	pipeline_builder._scissor.extent = _windowSize;

	pipeline_builder._rasterizer = vk_init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

	//we don't use multisampling, so just run the default one
	pipeline_builder._multisampling = vk_init::pipelineMultisampleStateCreateInfo();

	//a single blend attachment with no blending and writing to RGBA
	pipeline_builder._colorBlendAttachment = vk_init::pipelineColorBlendAttachmentState();


	pipeline_builder._depthStencil = vk_init::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);


	pipeline_builder._layout = _lightPipelineLayout;

	_lightPipeline = pipeline_builder.build(_device, _renderPass);


	vkDestroyShaderModule(_device, vertexShader, nullptr);
	vkDestroyShaderModule(_device, fragShader, nullptr);

	/* Object pipeline creation */

	vertexShaderPath = SHADER_DIR + std::string{"mesh.vert.spv"};
	fragShaderPath = SHADER_DIR + std::string{"mesh.frag.spv"};


	if (!vk_io::loadShaderModule(_device, vertexShaderPath.c_str(), &vertexShader)) {
		std::cerr << "Couldn't load vertex shader: " << vertexShaderPath << std::endl;
	}
	else {
		std::cout << "Loaded vertex shader: " << vertexShaderPath << std::endl;
	}

	if (!vk_io::loadShaderModule(_device, fragShaderPath.c_str(), &fragShader)) {
		std::cerr << "Couldn't load fragment shader: " << fragShaderPath << std::endl;
	}
	else {
		std::cout << "Loaded fragment shader: " << fragShaderPath << std::endl;
	}

	//Replace shaders
	pipeline_builder._shaderStages.clear();

	pipeline_builder._shaderStages.emplace_back(
		vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader)
	);

	pipeline_builder._shaderStages.emplace_back(
		vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
	);


	//Replace pipeline layout
	pipeline_layout_info = vk_init::pipelineLayoutCreateInfo();


	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &_objectDescriptorLayout;

	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_objectPipelineLayout));

	pipeline_builder._layout = _objectPipelineLayout;

	_objectPipeline = pipeline_builder.build(_device, _renderPass);

	vkDestroyShaderModule(_device, vertexShader, nullptr);
	vkDestroyShaderModule(_device, fragShader, nullptr);
}

void VkApp::initImgui()
{
	//Create descriptor pool for imgui resources
	VkDescriptorPoolSize pool_sizes[] =
	{
		/*{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }*/
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_imguiDescriptorPool));

	//Basic imgui setup	
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	//TODO: Multi-viewport rendering
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows


	//ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	//Vulkan specific setup
	//Install callbacks?	
	ImGui_ImplGlfw_InitForVulkan(_window, true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _gpu;
	init_info.Device = _device;
	init_info.QueueFamily = _graphicsFamilyQueueIndex;
	init_info.Queue = _graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = _imguiDescriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = NUM_FRAMES;
	init_info.ImageCount = NUM_FRAMES;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;

	ImGui_ImplVulkan_Init(&init_info, _renderPass);	

	//Upload imgui fonts to gpu
	immediateSubmit([&](VkCommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture();
	});

	//Destroy cpu side font data
	ImGui_ImplVulkan_DestroyFontsTexture();
}

void VkApp::initBuffers()
{

	/* Vertex buffers */
	/*_vertices = std::vector<vk_primitives::mesh::Vertex_F3_F3>{
		{ {1.0f,1.0f,0.0}, {0.0f,0.0f,0.0f} },
		{ {-1.0f,1.0f,0.0}, {1.0f,1.0f,1.0f} },
		{ {-1.0f,-1.0f,0.0}, {1.0f,0.0f,0.0f} },
		{ {1.0f,-1.0f,0.0}, {0.0f,1.0f,0.0f} },
	};*/

	//_vertices = vk_primitives::shapes::Cube::getVertexData();
	_vertices = vk_primitives::shapes::Cube::getNonIndexedVertexData();


	size_t vertex_buffer_size = _vertices.size() * sizeof(vk_primitives::mesh::Vertex_F3_F3_F2);
	//Create CPU side staging buffer
	auto vertexStagingBuffer = vk_util::createBuffer(_allocator, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	//Create GPU side vertex buffer
	_vertexBuffer = vk_util::createBuffer(_allocator, vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	//Copy data into CPU side staging buffer
	void* data;
	vmaMapMemory(_allocator, vertexStagingBuffer._allocation, &data);

	memcpy(data, _vertices.data(), vertex_buffer_size);

	vmaUnmapMemory(_allocator, vertexStagingBuffer._allocation);

	//Transfer data from CPU staging buffer to GPU side buffer
	immediateSubmit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.srcOffset = 0;
		copy.dstOffset = 0;
		copy.size = vertex_buffer_size;
		vkCmdCopyBuffer(cmd, vertexStagingBuffer._buffer, _vertexBuffer._buffer, 1, &copy);
	});
	
	//Cleanup temporary staging buffer
	vmaDestroyBuffer(_allocator, vertexStagingBuffer._buffer, vertexStagingBuffer._allocation);
	
	/* Index buffers */
	/*_indices = std::vector<uint32_t>{
		0, 2, 3,
		0, 1, 2
	};*/

	/*_indices = vk_primitives::shapes::Cube::getIndexData();

	size_t index_buffer_size = _indices.size() * sizeof(uint32_t);
	//Create CPU side staging buffer
	auto indexStagingBuffer = vk_util::createBuffer(_allocator, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	//Create GPU side index buffer
	_indexBuffer = vk_util::createBuffer(_allocator,index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	//Copy data into CPU side staging buffer
	vmaMapMemory(_allocator, indexStagingBuffer._allocation, &data);

	memcpy(data, _indices.data(), index_buffer_size);

	vmaUnmapMemory(_allocator, indexStagingBuffer._allocation);

	//Transfer data from CPU staging buffer to GPU side buffer
	immediateSubmit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.srcOffset = 0;
		copy.dstOffset = 0;
		copy.size = index_buffer_size;
		vkCmdCopyBuffer(cmd, indexStagingBuffer._buffer, _indexBuffer._buffer, 1, &copy);
		});

	//Cleanup temporary staging buffer
	vmaDestroyBuffer(_allocator, indexStagingBuffer._buffer,indexStagingBuffer._allocation);*/

	/* Uniform buffers */

	//Viewproj matrix per frame
	size_t camera_buffer_size = vk_util::padBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(GPUCameraData));
	camera_buffer_size *= NUM_FRAMES;

	_cameraBuffer = vk_util::createBuffer(_allocator, camera_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	size_t material_buffer_size = vk_util::padBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(MaterialEntity));

	//TODO: Make per frame
	_materialBuffer = vk_util::createBuffer(_allocator, material_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	vmaMapMemory(_allocator, _materialBuffer._allocation, &data);

	MaterialEntity* material = (MaterialEntity*)data;
	material->ambient = math::Vec4{ 0.3,0.0,0.0,0.0 };
	material->diffuse = math::Vec4{ 0.5,0,0.0,0 };
	material->specular = math::Vec4{1.0,1.0,1.0,0 };
	material->shiny = 0.5;


	vmaUnmapMemory(_allocator, _materialBuffer._allocation);

	/* Storage buffers */

	
	size_t light_buffer_size = vk_util::padBufferSize(_gpuProperties.limits.minStorageBufferOffsetAlignment, sizeof(LightEntity));
	light_buffer_size *= NUM_LIGHTS * NUM_FRAMES;

	_lightBuffer = vk_util::createBuffer(_allocator, light_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//Fill buffer with data
	vmaMapMemory(_allocator, _lightBuffer._allocation, &data);

	LightEntity* light = (LightEntity*)data;
	for (uint32_t i = 0; i < NUM_LIGHTS; i++) {
		for (uint32_t j = 0; j < NUM_FRAMES; j++) {
			light[i + j * NUM_LIGHTS].position = math::Vec4{ 4.0f * i, 6.0f, 2.0f * i,0.0 };
			//float r = (static_cast<float>(i) + 1) / NUM_LIGHTS;
			light[i + j * NUM_LIGHTS].ambient = math::Vec4{ 1,1,1,0 };
			light[i + j * NUM_LIGHTS].diffuse = math::Vec4{ 1,1,1,0 };
			light[i + j * NUM_LIGHTS].specular = math::Vec4{ 1,1,1,0 };
		}

	}
	vmaUnmapMemory(_allocator, _lightBuffer._allocation);

	size_t object_buffer_size  = vk_util::padBufferSize(_gpuProperties.limits.minStorageBufferOffsetAlignment, sizeof(RenderEntity));
	object_buffer_size *= NUM_OBJECTS;
	_objectBuffer = vk_util::createBuffer(_allocator, object_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	vmaMapMemory(_allocator, _objectBuffer._allocation, &data);

	RenderEntity* renderable = (RenderEntity*)data;
	float min_r = 10.0;
	float max_r = 25.0;
	float angle_speed = 32.0;

	for (uint32_t i = 0; i < NUM_OBJECTS; i++) {
		float dx = static_cast<float>(i + 1) / NUM_OBJECTS;
		float r = (1.0 - dx) * min_r + dx * max_r;
		float angle = dx * 2.0 * PI * angle_speed;

		renderable[i].model = math::Mat4::fromTranslation(r*cos(angle),0.0, r*sin(angle));
	}

	vmaUnmapMemory(_allocator, _objectBuffer._allocation);
}

void VkApp::initImages()
{
	std::string diffuse_img_path = IMAGE_DIR + std::string{"crate_diffuse_map.png"};
	std::string specular_img_path = IMAGE_DIR + std::string{"crate_specular_map.png"};

	if (vk_io::loadImage(*this, diffuse_img_path.c_str(), _diffuseImage)) {
		std::cout << "Loaded image: " << diffuse_img_path << std::endl;
	}

	if (vk_io::loadImage(*this, specular_img_path.c_str(), _specularImage)) {
		std::cout << "Loaded image: " << specular_img_path << std::endl;
	}


	//Create image view for diffuse and specular images
	VkImageViewCreateInfo create_info = vk_init::imageViewCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, _diffuseImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
	VK_CHECK(vkCreateImageView(_device, &create_info, nullptr, &_diffuseImageView));

	create_info = vk_init::imageViewCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, _specularImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
	VK_CHECK(vkCreateImageView(_device, &create_info, nullptr, &_specularImageView));

}

void VkApp::initDescriptors()
{

	/* Set 0 (light set) */
	VkDescriptorSetLayoutBinding light_bindings[] = 
	{ 
		//Binding 0 (View-proj per frame matrix)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 0),
		//Binding 1 (Light instance storage buffer)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1)
	};

	VkDescriptorSetLayoutCreateInfo set0_layout_info = vk_init::descriptorSetLayoutCreateInfo(2, light_bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(_device, &set0_layout_info, nullptr, &_lightDescriptorSetLayout));


	/* Set 1 (mesh set) */
	VkDescriptorSetLayoutBinding mesh_bindings[] =
	{
		//Binding 0 (View-proj per frame matrix)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		//Binding 1 (Light instance storage buffer)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		//Binding 2 (Object transforms)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 2),
		//Binding 3 (Material uniform)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
		//Binding 4 (Diffuse Texture)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
		//Binding 5 (Specular Texture)
		vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5)
	};

	VkDescriptorSetLayoutCreateInfo set1_layout_info = vk_init::descriptorSetLayoutCreateInfo(6, mesh_bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(_device, &set1_layout_info, nullptr, &_objectDescriptorLayout));

	//Create descriptor pool
	std::vector<VkDescriptorPoolSize> sizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,10},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,10},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,10},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,10}
	};

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext = nullptr;
	pool_info.maxSets = 10;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();

	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool));
	

	//Allocate descriptors from pool

	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.descriptorPool = _descriptorPool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &_lightDescriptorSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(_device, &alloc_info,&_lightDescriptorSet));

	alloc_info.pSetLayouts = &_objectDescriptorLayout;

	VK_CHECK(vkAllocateDescriptorSets(_device, &alloc_info, &_objectDescriptorSet));


	//Write to descriptor set
	VkDeviceSize bufferSize;

	//(Set 0,binding 0)
	bufferSize = vk_util::padBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(GPUCameraData));
	VkDescriptorBufferInfo buffer_info0_0 = vk_init::descriptorBufferInfo(_cameraBuffer._buffer, 0, bufferSize);

	//(Set 0,binding 1)
	//bufferSize = vk_util::padBufferSize(_gpuProperties.limits.minStorageBufferOffsetAlignment, sizeof(LightEntity))*NUM_LIGHTS*NUM_FRAMES;
	bufferSize = vk_util::padBufferSize(_gpuProperties.limits.minStorageBufferOffsetAlignment, sizeof(LightEntity)) * NUM_LIGHTS;

	std::cout << "sz: " << bufferSize << std::endl;
	VkDescriptorBufferInfo buffer_info0_1 = vk_init::descriptorBufferInfo(_lightBuffer._buffer, 0, bufferSize);

	//(Set 1,binding 0)
	VkDescriptorBufferInfo buffer_info1_0 = buffer_info0_0;

	//(Set 1,binding 1)
	VkDescriptorBufferInfo buffer_info1_1 = buffer_info0_1;

	//(Set 1,binding 2)
	bufferSize = vk_util::padBufferSize(_gpuProperties.limits.minStorageBufferOffsetAlignment, sizeof(RenderEntity)) * NUM_OBJECTS;
	VkDescriptorBufferInfo buffer_info1_2 = vk_init::descriptorBufferInfo(_objectBuffer._buffer, 0, bufferSize);

	//(Set 1,binding 3)
	bufferSize = vk_util::padBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(MaterialEntity));
	std::cout << "mat size: " << bufferSize << std::endl;
	VkDescriptorBufferInfo buffer_info1_3 = vk_init::descriptorBufferInfo(_materialBuffer._buffer, 0, bufferSize);

	//(Set 1,binding 4)
	VkDescriptorImageInfo img_info1_4{};
	img_info1_4.sampler = _blockySampler;
	img_info1_4.imageView = _diffuseImageView;
	img_info1_4.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	//(Set 1,binding 5)
	VkDescriptorImageInfo img_info1_5{};
	img_info1_5.sampler = _blockySampler;
	img_info1_5.imageView = _specularImageView;
	img_info1_5.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet writes[] = 
	{
		//Set 0
		vk_init::writeDescriptorBuffer(_lightDescriptorSet,0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,&buffer_info0_0),
		vk_init::writeDescriptorBuffer(_lightDescriptorSet,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,&buffer_info0_1),

		//Set 1
		vk_init::writeDescriptorBuffer(_objectDescriptorSet,0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,&buffer_info1_0),
		vk_init::writeDescriptorBuffer(_objectDescriptorSet,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,&buffer_info1_1),
		vk_init::writeDescriptorBuffer(_objectDescriptorSet,2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,&buffer_info1_2),
		vk_init::writeDescriptorBuffer(_objectDescriptorSet,3,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,&buffer_info1_3),
		vk_init::writeDescriptorImage(_objectDescriptorSet,4,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,&img_info1_4),
		vk_init::writeDescriptorImage(_objectDescriptorSet,5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,&img_info1_5)
	};

	vkUpdateDescriptorSets(_device, 8, writes, 0, nullptr);
}


void VkApp::destroyWindow()
{

	glfwDestroyWindow(_window);

	glfwTerminate();
}

void VkApp::destroyVulkan()
{
	vmaDestroyAllocator(_allocator);

	vkDestroyDevice(_device, nullptr);
	vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
	vkDestroyInstance(_instance, nullptr);
}

void VkApp::destroySwapchain()
{
	vkDestroyImageView(_device, _depthImageView, nullptr);
	vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);


	for (uint32_t i = 0; i < _swapchainImageViews.size(); i++) {
		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);
	vkDestroySurfaceKHR(_instance, _surface, nullptr);
}

void VkApp::destroyCommands()
{
	for (uint32_t i = 0; i < NUM_FRAMES; i++) {
		vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
	}

	vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
}

void VkApp::destroyRenderPasses()
{
	vkDestroyRenderPass(_device, _renderPass, nullptr);
}

void VkApp::destroyFrameBuffers()
{
	for (uint32_t i = 0; i < _frameBuffers.size(); i++) {
		vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
	}
}

void VkApp::destroySync()
{
	for (uint32_t i = 0; i < NUM_FRAMES; i++) {
		vkDestroyFence(_device, _frames[i]._renderDoneFence, nullptr);

		vkDestroySemaphore(_device, _frames[i]._imgReadyFlag, nullptr);
		vkDestroySemaphore(_device, _frames[i]._renderDoneFlag, nullptr);

	}

	vkDestroyFence(_device, _uploadContext._uploadDoneFence, nullptr);
}

void VkApp::destroySamplers()
{
	vkDestroySampler(_device, _blockySampler, nullptr);
}

void VkApp::destroyPipelines()
{
	vkDestroyPipelineLayout(_device, _lightPipelineLayout, nullptr);
	vkDestroyPipelineLayout(_device, _objectPipelineLayout, nullptr);
	vkDestroyPipeline(_device, _lightPipeline, nullptr);
	vkDestroyPipeline(_device, _objectPipeline, nullptr);
}

void VkApp::destroyImgui()
{
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(_device, _imguiDescriptorPool, nullptr);
}

void VkApp::drawUI()
{
	//Draw all imgui stuff here
	//ImGui::ShowDemoWindow();

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
	//window_flags |= ImGuiWindowFlags_NoBackground;

	if (ImGui::BeginMainMenuBar()) {


		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Phong Model Menu")) {
				//mViewer.mViewerOpen = !(mViewer.mViewerOpen);
			}
			ImGui::EndMenu();
		}


		ImGui::Text("TEXT\n");

		ImGui::EndMainMenuBar();


	}

	//ImGui::End();
}

void VkApp::destroyBuffers()
{
	vmaDestroyBuffer(_allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
	vmaDestroyBuffer(_allocator, _indexBuffer._buffer, _indexBuffer._allocation);
	vmaDestroyBuffer(_allocator, _cameraBuffer._buffer, _cameraBuffer._allocation);
	vmaDestroyBuffer(_allocator, _materialBuffer._buffer, _materialBuffer._allocation);
	vmaDestroyBuffer(_allocator, _lightBuffer._buffer, _lightBuffer._allocation);
	vmaDestroyBuffer(_allocator, _objectBuffer._buffer, _objectBuffer._allocation);
}

void VkApp::destroyImages()
{
	vkDestroyImageView(_device, _diffuseImageView, nullptr);
	vkDestroyImageView(_device, _specularImageView, nullptr);

	vmaDestroyImage(_allocator, _diffuseImage._image, _diffuseImage._allocation);
	vmaDestroyImage(_allocator, _specularImage._image, _specularImage._allocation);
}

void VkApp::destroyDescriptors()
{
	vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(_device, _lightDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(_device, _objectDescriptorLayout, nullptr);
}

RenderFrame& VkApp::getFrame()
{
	return _frames[_frameNum % NUM_FRAMES];
}

void VkApp::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = _uploadContext._commandBuffer;

	//Begin recording a new commmand to send
	VkCommandBufferBeginInfo begin_info = vk_init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd,&begin_info));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vk_init::submitInfo(&cmd);


	//Submit the command
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadDoneFence));

	//Have cpu wait for command to finish before proceeding
	vkWaitForFences(_device, 1, &_uploadContext._uploadDoneFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadDoneFence);

	// reset the command buffers inside the command pool
	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

void _cursorMotionCallback(GLFWwindow* window, double xpos, double ypos)
{
	VkApp* app = static_cast<VkApp*>(glfwGetWindowUserPointer(window));

	if (app->_mousePressed) {
		if (!app->_mouseInit) {
			app->_mouseX = xpos;
			app->_mouseY = ypos;

			app->_mouseInit = true;
		}
		else {
			float dx = xpos - app->_mouseX;
			float dy = app->_mouseY - ypos;
			app->_mouseX = xpos;
			app->_mouseY = ypos;

			app->_mainCamera.rotateTheta(dx * -0.005);
			app->_mainCamera.rotatePhi(-dy * 0.01);
		}
	}
	else {
		app->_mouseInit = false;
	}
}

void _mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	VkApp* app = static_cast<VkApp*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			app->_mousePressed = true;
		}
		else if (action == GLFW_RELEASE) {
			app->_mousePressed = false;
		}
	}
}
