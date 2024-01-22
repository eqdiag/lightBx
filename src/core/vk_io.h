#pragma once

#include "vk_types.h"
#include "vk_log.h"
#include "vk_app.h"

namespace vk_io {

	bool loadShaderModule(VkDevice device, const char* filePath, VkShaderModule* shaderModule);

	bool loadImage(VkApp& app,const char* filePath, vk_types::AllocatedImage& image);
}