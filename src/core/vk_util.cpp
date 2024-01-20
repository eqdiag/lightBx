#include "vk_util.h"
#include "vk_log.h"

vk_types::AllocatedBuffer vk_util::createBuffer(VmaAllocator allocator, size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage)
{
	vk_types::AllocatedBuffer buffer{};

	VkBufferCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.size = size;
	create_info.usage = usage;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage = memUsage;

	VK_CHECK(vmaCreateBuffer(allocator, &create_info, &alloc_info, &buffer._buffer, &buffer._allocation, nullptr));

	return buffer;
}

size_t vk_util::padBufferSize(size_t alignment, size_t originalSize)
{
	// Calculate required alignment based on minimum device offset alignment
	size_t minAlignment = alignment;
	size_t alignedSize = originalSize;
	if (minAlignment > 0) {
		alignedSize = (alignedSize + minAlignment - 1) & ~(minAlignment - 1);
	}
	return alignedSize;
}
