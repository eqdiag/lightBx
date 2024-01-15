#include "vk_init.h"

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
