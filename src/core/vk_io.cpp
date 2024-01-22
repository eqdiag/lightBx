#include "vk_io.h"
#include "vk_util.h"
#include "vk_init.h"

#include <fstream>
#include <vector>

#include "stb_image.h"


bool vk_io::loadShaderModule(VkDevice device, const char* filePath, VkShaderModule* shaderModule)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	size_t fileSize = (size_t)file.tellg();

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	//put file cursor at beginning
	file.seekg(0);

	//load file into buffer
	file.read((char*)buffer.data(), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = nullptr;

	create_info.codeSize = buffer.size() * sizeof(uint32_t);
	create_info.pCode = buffer.data();

	//check that the creation goes well.
	VkShaderModule newShaderModule;
	if (vkCreateShaderModule(device, &create_info, nullptr, &newShaderModule) != VK_SUCCESS) {
		return false;
	}
	*shaderModule = newShaderModule;
	return true;
}

bool vk_io::loadImage(VkApp& app, const char* filePath, vk_types::AllocatedImage& image)
{

	int width, height, num_channels;

	stbi_uc* data = stbi_load(filePath, &width, &height, &num_channels, STBI_rgb_alpha);

	if (!data) {
		std::cout << "Failed to load: " << filePath << std::endl;
		return false;
	}

	//Put image data in staging buffer

	void* pixels = (void*)data;
	VkDeviceSize size = width * height * 4;

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	vk_types::AllocatedBuffer staging_buffer = vk_util::createBuffer(app._allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* mem;
	vmaMapMemory(app._allocator, staging_buffer._allocation, &mem);

	memcpy(mem, pixels, static_cast<size_t>(size));

	vmaUnmapMemory(app._allocator, staging_buffer._allocation);

	stbi_image_free(pixels);

	VkExtent3D extent;
	extent.width = static_cast<uint32_t>(width);
	extent.height = static_cast<uint32_t>(height);
	extent.depth = 1;

	//Allocate gpu only image 

	VkImageCreateInfo create_info = vk_init::imageCreateInfo(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, extent);

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(app._allocator, &create_info, &alloc_info, &image._image, &image._allocation, nullptr));

	app.immediateSubmit([=](VkCommandBuffer cmd) {
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier imageBarrier_toTransfer = {};
		imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier_toTransfer.image = image._image;
		imageBarrier_toTransfer.subresourceRange = range;

		imageBarrier_toTransfer.srcAccessMask = 0;
		imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		//barrier the image into the transfer-receive layout
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

		//Copy buffer data to image
		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = extent;

		//copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, staging_buffer._buffer, image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		//Change layout one more time

		VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

		imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		//barrier the image into the shader readable layout
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
	});

	vmaDestroyBuffer(app._allocator, staging_buffer._buffer, staging_buffer._allocation);

	return true;
}
