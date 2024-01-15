#include "vk_app.h"
#include "vk_log.h"
#include "vk_init.h"
#include "settings.h"
#include "VkBootstrap.h"

void VkApp::init()
{
	initWindow();

	initVulkan();

	initSwapchain();

	initCommands();

	initSync();

	initRenderPasses();

	initFrameBuffers();

	initPipelines();

	_init = true;

}

void VkApp::run()
{
	while (!glfwWindowShouldClose(_window)) {

		glfwPollEvents();

		draw();

	}
}

void VkApp::draw()
{
	//Get current frame data
	auto& frame = getFrame();

	VK_CHECK(vkWaitForFences(_device, 1, &frame._renderDoneFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &frame._renderDoneFence));

	uint32_t nextImgIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, frame._imgReadyFlag, VK_NULL_HANDLE, &nextImgIndex));

	VkCommandBuffer cmd = frame._commandBuffer;

	//Reset command buffer
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = nullptr;

	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	VkClearValue clearValue;
	float flash = abs(sin(_frameNum / 120.0f));
	clearValue.color = { {0,flash,0,1.0f} };

	VkRenderPassBeginInfo pass_begin_info{};
	pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass_begin_info.pNext = nullptr;

	pass_begin_info.renderPass = _renderPass;
	pass_begin_info.renderArea.offset.x = 0;
	pass_begin_info.renderArea.offset.y = 0;
	pass_begin_info.renderArea.extent = _windowSize;
	pass_begin_info.framebuffer = _frameBuffers[nextImgIndex];

	pass_begin_info.clearValueCount = 1;
	pass_begin_info.pClearValues = &clearValue;

	vkCmdBeginRenderPass(cmd, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	vkCmdDraw(cmd, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

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


		destroySync();

		destroyPipelines();

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

	vkb::Device device = device_builder
		.build().value();

	_device = device.device;

	_graphicsFamilyQueueIndex = device.get_queue_index(vkb::QueueType::graphics).value();
	_graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
}

void VkApp::initSwapchain()
{
	vkb::SwapchainBuilder swapchain_builder{_gpu, _device, _surface};

	vkb::Swapchain swapchain = swapchain_builder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_windowSize.width,_windowSize.height)
		.build()
		.value();

	_swapchain = swapchain.swapchain;
	_swapchainFormat = swapchain.image_format;
	_swapchainImages = swapchain.get_images().value();
	_swapchainImageViews = swapchain.get_image_views().value();
}

void VkApp::initCommands()
{
	//Enable buffer resetting
	VkCommandPoolCreateInfo pool_create_info = vk_init::commandPoolCreateInfo(_graphicsFamilyQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);


	for (uint32_t i = 0; i < NUM_FRAMES; i++) {

		VK_CHECK(vkCreateCommandPool(_device, &pool_create_info, nullptr, &_frames[i]._commandPool));

		//Allocate 1 primary command buffer
		VkCommandBufferAllocateInfo buffer_alloc_info = vk_init::commandBufferAllocateInfo(_frames[i]._commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VK_CHECK(vkAllocateCommandBuffers(_device, &buffer_alloc_info, &_frames[i]._commandBuffer));
	}
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

	/* Subpasses */
	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	/* Dependencies */

	/* Render pass */

	VkRenderPassCreateInfo pass_create_info{};
	pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	pass_create_info.pNext = nullptr;

	pass_create_info.attachmentCount = 1;
	pass_create_info.pAttachments = &color_attachment;

	pass_create_info.subpassCount = 1;
	pass_create_info.pSubpasses = &subpass;

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
		create_info.pAttachments = &_swapchainImageViews[i];
		VK_CHECK(vkCreateFramebuffer(_device,&create_info,nullptr,&_frameBuffers[i]));
	}

}

void VkApp::initSync()
{
	//Start fence in signaled state
	VkFenceCreateInfo fence_create_info = vk_init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

	VkSemaphoreCreateInfo semaphore_create_info = vk_init::semaphoreCreateInfo();

	for (uint32_t i = 0; i < NUM_FRAMES; i++) {

		VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[i]._renderDoneFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._imgReadyFlag));
		VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._renderDoneFlag));
	}
}

void VkApp::initPipelines()
{
	VkShaderModule vertexShader{};
	VkShaderModule fragShader{};

	std::string vertexShaderPath = SHADER_DIR + std::string{"mesh.vert.spv"};
	std::string fragShaderPath = SHADER_DIR + std::string{"mesh.frag.spv"};


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

	VkPipelineLayoutCreateInfo pipeline_layout_info = vk_init::pipelineLayoutCreateInfo();

	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_pipelineLayout));

	pipeline_builder._vertexInputInfo = vk_init::pipelineVertexInputStateCreateInfo();

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


	pipeline_builder._layout = _pipelineLayout;

	_pipeline = pipeline_builder.build(_device, _renderPass);


	vkDestroyShaderModule(_device, vertexShader, nullptr);
	vkDestroyShaderModule(_device, fragShader, nullptr);
}


void VkApp::destroyWindow()
{

	glfwDestroyWindow(_window);

	glfwTerminate();
}

void VkApp::destroyVulkan()
{
	vkDestroyDevice(_device, nullptr);
	vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
	vkDestroyInstance(_instance, nullptr);
}

void VkApp::destroySwapchain()
{
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
}

void VkApp::destroyPipelines()
{
	vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
	vkDestroyPipeline(_device, _pipeline, nullptr);
}

RenderFrame& VkApp::getFrame()
{
	return _frames[_frameNum % NUM_FRAMES];
}


