#include "vk_texture.h"

#include "stb_image.h"
#include "vk_log.h"

bool loadImgFromFile(VkEngine& engine, const char* fileName, AllocatedImage& outImage)
{
	int width, height, numChannels;

	stbi_uc* pixels = stbi_load(fileName, &width, &height, &numChannels, STBI_rgb_alpha);

	if (!pixels) {
		std::cerr << "ERROR: Failed to load image file: " << fileName << std::endl;
	}

	//Create staging buffer
	void* pixel_start = (void*)pixels;
	VkDeviceSize img_size = width * height * 4;

	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

	//Create cpu side staging buffer
	AllocatedBuffer stagingBuffer{};
	vk_init::createBuffer(img_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,engine._allocator,&stagingBuffer);

	//Copy image data into staging buffer
	void* data;
	vmaMapMemory(engine._allocator, stagingBuffer._allocation, &data);

	memcpy(data, pixel_start, static_cast<uint32_t>(img_size));

	vmaUnmapMemory(engine._allocator, stagingBuffer._allocation);

	stbi_image_free(pixels);

	//Now for gpu side stuff

	//Create actual image

	VkExtent3D extent{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	};

	VkImageCreateInfo image_create_info = vk_init::imageCreateInfo(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, extent);

	AllocatedImage gpuImage;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(engine._allocator, &image_create_info, &alloc_info, &gpuImage._image, &gpuImage._allocation, nullptr));

	//Now copy stuff over from staging buffer

	engine.immediate_submit([=](VkCommandBuffer cmd) {
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		//Have to convert image into a layout before data transfer (unlike buffers)

		VkImageMemoryBarrier img_barrier{};
		img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		img_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		img_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		img_barrier.image = gpuImage._image;
		img_barrier.subresourceRange = range;

		img_barrier.srcAccessMask = 0;
		//Dont' write to this image until the pipeline barrier executions has finished
		img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		//Wait on img transition
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &img_barrier);


		VkBufferImageCopy copyRegion{};
		//Src buffer info
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		//Dst img info
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = extent;

		vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, gpuImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		//Now we change the image format again so that it's acceptable to read from shaders

		VkImageMemoryBarrier img_barrier_readable = img_barrier;

		//Type conversion/usage for image
		img_barrier_readable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		img_barrier_readable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		//Wait on all writes to this image
		img_barrier_readable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		img_barrier_readable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &img_barrier_readable);
	});


	vmaDestroyBuffer(engine._allocator, stagingBuffer._buffer, stagingBuffer._allocation);

	std::cout << "Loaded texture: " << fileName << std::endl;

	outImage = gpuImage;

	return true;
}
