#include "vk_app.h"
#include "vk_log.h"
#include "vk_init.h"
#include "vk_util.h"
#include "settings.h"
#include "VkBootstrap.h"

#include "primitives/mesh.h"
#include "primitives/camera.h"


#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void VkApp::init()
{
	initWindow();

	initVulkan();

	initSwapchain();

	initCommands();

	initSync();

	initRenderPasses();

	initFrameBuffers();

	initBuffers();

	initDescriptors();

	initPipelines();


	_init = true;

}

void VkApp::run()
{
	while (!glfwWindowShouldClose(_window)) {

		glfwPollEvents();

		draw();

		if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS) {
			_mainCamera.translate(-0.01, 0.0, 0.0);
		}else if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS) {
			_mainCamera.translate(0.01, 0.0, 0.0);
		}else if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS) {
			_mainCamera.translate(0.0, 0.0, -0.01);
		}else if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS) {
			_mainCamera.translate(0.0, 0.0, 0.01);
		}



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
	vk_primitives::camera::GPUCameraData gpu_data{};
	gpu_data.view_proj = proj * view;

	//Copy to gpu
	uint32_t offsetSize = vk_util::padUniformBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(vk_primitives::camera::GPUCameraData));
	char* data;
	vmaMapMemory(_allocator, _cameraBuffer._allocation, (void**)&data);

	data += offsetSize * frameIdx;
	memcpy(data, (void*)&gpu_data, offsetSize);

	vmaUnmapMemory(_allocator, _cameraBuffer._allocation);


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
	//float flash = abs(sin(_frameNum / 120.0f));
	clearValue.color = { {0,0,0,1.0f} };

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

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &_vertexBuffer._buffer, &offset);


	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_frameDescriptorSet, 1, &offsetSize);

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

		destroyPipelines();

		destroyDescriptors();

		destroyBuffers();

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

	/* Pipeline layout */

	VkPipelineLayoutCreateInfo pipeline_layout_info = vk_init::pipelineLayoutCreateInfo();

	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &_frameDescriptorLayout;

	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_pipelineLayout));

	/* Vertex input */

	VkPipelineVertexInputStateCreateInfo vertex_input_info = vk_init::pipelineVertexInputStateCreateInfo();

	auto description = vk_primitives::mesh::Vertex_F3_F3::getVertexInputDescription();

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


	pipeline_builder._layout = _pipelineLayout;

	_pipeline = pipeline_builder.build(_device, _renderPass);


	vkDestroyShaderModule(_device, vertexShader, nullptr);
	vkDestroyShaderModule(_device, fragShader, nullptr);
}

void VkApp::initBuffers()
{

	/* Vertex buffers */
	std::vector<vk_primitives::mesh::Vertex_F3_F3> vertices{
		{ {1.0f,1.0f,0.0}, {1.0f,0.0f,0.0f} },
		{ {-1.0f,1.0f,0.0}, {0.0f,1.0f,0.0f} },
		{ {0.0f,-1.0f,0.0}, {0.0f,0.0f,1.0f} }
	};

	//Create vertex buffer
	_vertexBuffer = vk_util::createBuffer(_allocator, vertices.size() * sizeof(vk_primitives::mesh::Vertex_F3_F3), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	
	//Copy data into buffer

	void* data;
	vmaMapMemory(_allocator, _vertexBuffer._allocation, &data);

	memcpy(data, vertices.data(), vertices.size() * sizeof(vk_primitives::mesh::Vertex_F3_F3));

	vmaUnmapMemory(_allocator, _vertexBuffer._allocation);

	/* Uniform buffers */

	//Viewproj matrix per frame
	size_t camera_buffer_size = vk_util::padUniformBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(vk_primitives::camera::GPUCameraData));
	camera_buffer_size *= NUM_FRAMES;

	_cameraBuffer = vk_util::createBuffer(_allocator, camera_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void VkApp::initDescriptors()
{
	/* Set 0 */

	//Binding 0
	VkDescriptorSetLayoutBinding camera_binding{};
	camera_binding.binding = 0;
	camera_binding.descriptorCount = 1;
	camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	camera_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Layout
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.pNext = nullptr;
	
	layout_info.bindingCount = 1;
	layout_info.flags = 0;
	layout_info.pBindings = &camera_binding;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &layout_info, nullptr, &_frameDescriptorLayout));

	//Create descriptor pool

	std::vector<VkDescriptorPoolSize> sizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,10}
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
	alloc_info.pSetLayouts = &_frameDescriptorLayout;

	VK_CHECK(vkAllocateDescriptorSets(_device, &alloc_info, &_frameDescriptorSet));

	//Write to descriptor set

	//(Set 0,binding 0)
	VkDescriptorBufferInfo buffer_info{};
	buffer_info.buffer = _cameraBuffer._buffer;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(vk_primitives::camera::GPUCameraData);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;

	write.dstBinding = 0;
	write.dstSet = _frameDescriptorSet;

	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
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

void VkApp::destroyBuffers()
{
	vmaDestroyBuffer(_allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
	vmaDestroyBuffer(_allocator, _cameraBuffer._buffer, _cameraBuffer._allocation);
}

void VkApp::destroyDescriptors()
{
	vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(_device, _frameDescriptorLayout, nullptr);
}

RenderFrame& VkApp::getFrame()
{
	return _frames[_frameNum % NUM_FRAMES];
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
