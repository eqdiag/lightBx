#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

namespace vk_types {

	struct AllocatedBuffer {
		VkBuffer _buffer;
		VmaAllocation _allocation;
	};
	
	struct AllocatedImage {
		VkImage _image;
		VmaAllocation _allocation;
	};
}