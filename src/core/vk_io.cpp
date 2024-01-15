#include "vk_io.h"

#include <fstream>
#include <vector>

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
