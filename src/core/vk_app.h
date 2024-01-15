#pragma once

#include "vk_types.h"

#include <vector>

constexpr uint32_t NUM_FRAMES = 2;

struct RenderFrame {
	/* Commands */
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
	
	/* Sync */
	VkFence _renderDoneFence;
	VkSemaphore _imgReadyFlag;
	VkSemaphore _renderDoneFlag;
};

class VkApp {

public:

	void init();
	void run();
	void draw();
	void cleanup();

private:

	/* INIT */

	void initWindow();

	void initVulkan();

	void initSwapchain();

	void initCommands();

	void initRenderPasses();

	void initFrameBuffers();

	void initSync();

	void initPipelines();


	/* DESTROY */

	void destroyWindow();

	void destroyVulkan();

	void destroySwapchain();

	void destroyCommands();

	void destroyRenderPasses();

	void destroyFrameBuffers();

	void destroySync();

	void destroyPipelines();

	/* Helpers */

	RenderFrame& getFrame();

	/* App State */
	bool _init{false};
	uint32_t _frameNum{ 0 };

	/* Window */
	GLFWwindow* _window;
	VkExtent2D _windowSize{ 800,600 };

	/* Instance/Debug messenger */
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;

	/* Swapchain */
	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	/* Device */
	VkPhysicalDevice _gpu;
	VkDevice _device;

	/* Queues */
	uint32_t _graphicsFamilyQueueIndex;
	VkQueue _graphicsQueue;

	/* Render passes */
	VkRenderPass _renderPass;

	/* Framebuffers */
	std::vector<VkFramebuffer> _frameBuffers;

	/* Pipelines */


	/* Resources (Buffers/Images) */

	/* Frames */
	RenderFrame _frames[NUM_FRAMES];

	/* Commands */

	/* Sync */

	/* Desciptors */


};