#pragma once

#include "vk_types.h"
#include "vk_mem_alloc.h"


namespace vk_util {


	vk_types::AllocatedBuffer createBuffer(VmaAllocator allocator,size_t size,VkBufferUsageFlags usage,VmaMemoryUsage memUsage);

	size_t padUniformBufferSize(size_t alignment, size_t originalSize);

}