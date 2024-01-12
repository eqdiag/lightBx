#pragma once

#include "core/vk_types.h"
#include "core/vk_mesh.h"

namespace vk_io{

    bool loadShaderModule(VkDevice device, const char* filePath,VkShaderModule* shaderModule);

    bool loadMeshFromFile(const char* fileName, Mesh& mesh);
}