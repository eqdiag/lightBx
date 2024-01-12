#include "vk_mesh.h"

VertexInputDescription Vertex::getVertexDescription()
{
	VertexInputDescription desc{};

	/* Bindings */

	VkVertexInputBindingDescription buffBinding{};
	buffBinding.binding = 0;
	//Read a "Vertex" at a time via binding
	buffBinding.stride = sizeof(Vertex);
	buffBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	desc.bindings.emplace_back(buffBinding);

	/* Attributes */

	VkVertexInputAttributeDescription posAttribute{};
	posAttribute.binding = 0;
	posAttribute.location = 0;
	posAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	posAttribute.offset = offsetof(Vertex, position);

	VkVertexInputAttributeDescription colAttribute{};
	colAttribute.binding = 0;
	colAttribute.location = 1;
	colAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	colAttribute.offset = offsetof(Vertex, color);

	VkVertexInputAttributeDescription normalAttribute{};
	normalAttribute.binding = 0;
	normalAttribute.location = 2;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, normal);

	VkVertexInputAttributeDescription texAttribute{};
	texAttribute.binding = 0;
	texAttribute.location = 3;
	texAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	texAttribute.offset = offsetof(Vertex, uv);

	desc.attributes.emplace_back(posAttribute);
	desc.attributes.emplace_back(colAttribute);
	desc.attributes.emplace_back(normalAttribute);
	desc.attributes.emplace_back(texAttribute);


	return desc;
}
