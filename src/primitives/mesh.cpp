#include "mesh.h"

vk_primitives::mesh::VertexInputDescription vk_primitives::mesh::Vertex_F3_F3::getVertexInputDescription()
{
	VertexInputDescription description{};

	/* Bindings */
	VkVertexInputBindingDescription binding0{};
	binding0.binding = 0;
	binding0.stride = sizeof(Vertex_F3_F3);
	binding0.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description._bindingDescriptions.emplace_back(binding0);

	/* Attributes */

	VkVertexInputAttributeDescription position{};
	position.binding = 0;
	position.location = 0;
	position.format = VK_FORMAT_R32G32B32_SFLOAT;
	position.offset = offsetof(Vertex_F3_F3, position);

	VkVertexInputAttributeDescription color{};
	color.binding = 0;
	color.location = 1;
	color.format = VK_FORMAT_R32G32B32_SFLOAT;
	color.offset = offsetof(Vertex_F3_F3, color);

	description._attributeDescriptions.emplace_back(position);
	description._attributeDescriptions.emplace_back(color);

	return description;
}
