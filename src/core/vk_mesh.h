#pragma once

#include "vk_types.h"
#include <vector>
#include <math/vec.h>


struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
	
	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
	math::Vec3 position;
	math::Vec3 color;
	math::Vec3 normal;
	math::Vec2 uv;

	static VertexInputDescription getVertexDescription();
};

struct Mesh {
	//Cpu side vertex data
	std::vector<Vertex> _vertices;

	//Gpu side vertex data
	AllocatedBuffer _vertexBuffer;
};