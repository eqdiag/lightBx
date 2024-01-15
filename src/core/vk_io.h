#pragma once

#include "vk_types.h"
#include "vk_log.h"

namespace vk_io {

	bool loadShaderModule(VkDevice device, const char* filePath, VkShaderModule* shaderModule);

}