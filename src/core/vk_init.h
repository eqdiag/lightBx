#pragma once

#include "vk_types.h"

namespace vk_init {

	/* Commands */
	VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	
	VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	/* Sync */

	VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

	/* Desciptors */

	/* Pipelines */

	/* Buffers */

	/* Images */

}