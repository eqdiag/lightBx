#pragma once

#include "vk_types.h"
#include "vk_io.h"

#include "primitives/mesh.h"
#include "primitives/camera.h"

#include "vk_mem_alloc.h"

#include <vector>
#include <functional>

constexpr uint32_t NUM_FRAMES = 2;
constexpr uint32_t NUM_LIGHTS = 4;
constexpr uint32_t NUM_OBJECTS = 10;

/* Frame */
struct RenderFrame {
	/* Commands */
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;

	/* Sync */
	VkFence _renderDoneFence;
	VkSemaphore _imgReadyFlag;
	VkSemaphore _renderDoneFlag;
};

/* Camera */
struct GPUCameraData {
	math::Mat4 view_proj;
};

/* Material */
struct MaterialEntity {
	math::Vec4 ambient;
	math::Vec4 diffuse;
	math::Vec4 specular;
	float shiny;
};

/* Light */
struct LightEntity{
	math::Vec4 position;
	math::Vec4 ambient;
	math::Vec4 diffuse;
	math::Vec4 specular;
};

/* Objects */
struct RenderEntity {
	math::Mat4 model;
};


struct UploadContext {
	VkFence _uploadDoneFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

class VkApp {

public:

	void init();
	void run();
	void draw();
	void cleanup();


	/* Input state */
	bool _mouseInit{ false };
	bool _mousePressed{ false };
	double _mouseX{};
	double _mouseY{};
	static constexpr float TRANSLATE_SPEED = 0.25;

	/* Camera */
	vk_primitives::camera::FlyCamera _mainCamera{math::Vec3{0, 0, -4}};


private:

	/* INIT */

	void initWindow();

	void initVulkan();

	void initSwapchain();

	void initCommands();

	void initRenderPasses();

	void initFrameBuffers();

	void initSync();

	void initBuffers();

	void initDescriptors();

	void initPipelines();

	void initImgui();

	/* DESTROY */

	void destroyWindow();

	void destroyVulkan();

	void destroySwapchain();

	void destroyCommands();

	void destroyRenderPasses();

	void destroyFrameBuffers();

	void destroySync();

	void destroyBuffers();

	void destroyDescriptors();

	void destroyPipelines();

	void destroyImgui();

	/* UI drawing */
	void drawUI();

	/* Helpers */

	RenderFrame& getFrame();

	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	/* App State */
	bool _init{false};
	uint32_t _frameNum{ 0 };

	/* Window */
	GLFWwindow* _window;
	VkExtent2D _windowSize{ 1200,800 };

	/* Instance/Debug messenger */
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;

	/* Swapchain */
	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	/* Depth buffer */
	VkFormat _depthFormat;
	vk_types::AllocatedImage _depthImage;
	VkImageView _depthImageView;

	/* Device */
	VkPhysicalDevice _gpu;
	VkPhysicalDeviceProperties _gpuProperties;
	VkDevice _device;

	/* VMA Allocator */
	VmaAllocator _allocator;

	/* Queues */
	uint32_t _graphicsFamilyQueueIndex;
	VkQueue _graphicsQueue;

	/* Render passes */
	VkRenderPass _renderPass;

	/* Framebuffers */
	std::vector<VkFramebuffer> _frameBuffers;

	/* Pipelines */
	//Light pipeline
	VkPipelineLayout _lightPipelineLayout;
	VkPipeline _lightPipeline;

	VkPipelineLayout _objectPipelineLayout;
	VkPipeline _objectPipeline;

	//Object pipeline


	/* Frames */
	RenderFrame _frames[NUM_FRAMES];

	/* Upload */
	UploadContext _uploadContext;

	/* Commands */

	/* Sync */


	/* Buffers */
	std::vector<vk_primitives::mesh::Vertex_F3_F3> _vertices;
	std::vector<uint32_t> _indices;

	//Vertex buffers
	vk_types::AllocatedBuffer _vertexBuffer;

	//Index buffers
	vk_types::AllocatedBuffer _indexBuffer;

	//Uniforms buffers
	vk_types::AllocatedBuffer _cameraBuffer;
	vk_types::AllocatedBuffer _materialBuffer;

	//Storage buffers
	vk_types::AllocatedBuffer _lightBuffer;
	vk_types::AllocatedBuffer _objectBuffer;

	/* Desciptors */
	VkDescriptorPool _descriptorPool;
	VkDescriptorPool _imguiDescriptorPool;

	//Dynamic descriptor set changed on frame scope
	VkDescriptorSetLayout _lightDescriptorSetLayout;
	VkDescriptorSet _lightDescriptorSet;

	VkDescriptorSetLayout _objectDescriptorLayout;
	VkDescriptorSet _objectDescriptorSet;


};

/* INPUT HANDLING */

void _cursorMotionCallback(GLFWwindow* window, double xpos, double ypos);

void _mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);