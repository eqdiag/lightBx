#include "vk_app.h"
#include "vk_log.h"
#include "vk_init.h"
#include "vk_util.h"
#include "settings.h"
#include "VkBootstrap.h"

#include "primitives/mesh.h"
#include "primitives/camera.h"
#include "primitives/shapes.h"


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
	gpu_data.matrix = proj * view;

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

	VkCommandBufferBeginInfo begin_info = vk_init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

	VkClearValue clearValue;
	//float flash = abs(sin(_frameNum / 120.0f));
	clearValue.color = { {0,0,0,1.0f} };

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

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

	//Bind vertex buffer
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &_vertexBuffer._buffer, &offset);

	//Bind index buffer
	vkCmdBindIndexBuffer(cmd, _indexBuffer._buffer, offset, VK_INDEX_TYPE_UINT32);

	//View/proj
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_frameDescriptorSet, 1, &offsetSize);
	//Model
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 1, 1, &_objectDescriptorSet, 0, nullptr);

	for (uint32_t i = 0; i < NUM_OBJECTS; i++) {
		//vkCmdDraw(cmd, 3, 1, 0, i);
		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(_indices.size()), 1, 0, 0, i);
	}

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

	VkDescriptorSetLayout layouts[] = { _frameDescriptorLayout,_objectDescriptorLayout };

	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = layouts;

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


	pipeline_builder._depthStencil = vk_init::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);


	pipeline_builder._layout = _pipelineLayout;

	_pipeline = pipeline_builder.build(_device, _renderPass);


	vkDestroyShaderModule(_device, vertexShader, nullptr);
	vkDestroyShaderModule(_device, fragShader, nullptr);
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

	_vertices = vk_primitives::shapes::Cube::getVertexData();



	size_t vertex_buffer_size = _vertices.size() * sizeof(vk_primitives::mesh::Vertex_F3_F3);
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

	_indices = vk_primitives::shapes::Cube::getIndexData();

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
	vmaDestroyBuffer(_allocator, indexStagingBuffer._buffer,indexStagingBuffer._allocation);

	/* Uniform buffers */

	//Viewproj matrix per frame
	size_t camera_buffer_size = vk_util::padUniformBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(vk_primitives::camera::GPUCameraData));
	camera_buffer_size *= NUM_FRAMES;

	_cameraBuffer = vk_util::createBuffer(_allocator, camera_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	/* Storage buffers */


	size_t model_buffer_size = sizeof(vk_primitives::camera::GPUCameraData);
	model_buffer_size *= NUM_OBJECTS;

	_modelBuffer = vk_util::createBuffer(_allocator, model_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//Fill buffer with data
	vmaMapMemory(_allocator, _modelBuffer._allocation, &data);

	vk_primitives::camera::GPUCameraData* cam_data = (vk_primitives::camera::GPUCameraData*)data;
	for (uint32_t i = 0; i < NUM_OBJECTS; i++) {
		cam_data[i].matrix = math::Mat4::fromTranslation(2.0 * i, 0, -10.0 + 2*i);
		//cam_data[i].matrix = math::Mat4::identity();
	}

	vmaUnmapMemory(_allocator, _modelBuffer._allocation);
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

	/* Set 1*/

	//Binding 0
	VkDescriptorSetLayoutBinding model_binding{};
	model_binding.binding = 0;
	model_binding.descriptorCount = 1;
	model_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	model_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Layout
	layout_info.pBindings = &model_binding;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &layout_info, nullptr, &_objectDescriptorLayout));


	//Create descriptor pool

	std::vector<VkDescriptorPoolSize> sizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,10},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,10}
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


	VK_CHECK(vkAllocateDescriptorSets(_device, &alloc_info,&_frameDescriptorSet));

	alloc_info.pSetLayouts = &_objectDescriptorLayout;

	VK_CHECK(vkAllocateDescriptorSets(_device, &alloc_info, &_objectDescriptorSet));


	//Write to descriptor set

	//(Set 0,binding 0)
	VkDescriptorBufferInfo buffer_info0{};
	buffer_info0.buffer = _cameraBuffer._buffer;
	buffer_info0.offset = 0;
	buffer_info0.range = vk_util::padUniformBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(vk_primitives::camera::GPUCameraData));

	//(Set 1,binding 0)
	VkDescriptorBufferInfo buffer_info1{};
	buffer_info1.buffer = _modelBuffer._buffer;
	buffer_info1.range = sizeof(vk_primitives::camera::GPUCameraData) * NUM_OBJECTS;

	VkWriteDescriptorSet write0{};
	write0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write0.pNext = nullptr;

	write0.dstBinding = 0;
	write0.dstSet = _frameDescriptorSet;

	write0.descriptorCount = 1;
	write0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write0.pBufferInfo = &buffer_info0;

	VkWriteDescriptorSet write1{};
	write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write1.pNext = nullptr;

	write1.dstBinding = 0;
	write1.dstSet = _objectDescriptorSet;

	write1.descriptorCount = 1;
	write1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write1.pBufferInfo = &buffer_info1;

	VkWriteDescriptorSet writes[] = { write0,write1 };

	vkUpdateDescriptorSets(_device, 2, writes, 0, nullptr);
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

void VkApp::destroyPipelines()
{
	vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
	vkDestroyPipeline(_device, _pipeline, nullptr);
}

void VkApp::destroyBuffers()
{
	vmaDestroyBuffer(_allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
	vmaDestroyBuffer(_allocator, _indexBuffer._buffer, _indexBuffer._allocation);
	vmaDestroyBuffer(_allocator, _cameraBuffer._buffer, _cameraBuffer._allocation);
	vmaDestroyBuffer(_allocator, _modelBuffer._buffer, _modelBuffer._allocation);
}

void VkApp::destroyDescriptors()
{
	vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(_device, _frameDescriptorLayout, nullptr);
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
