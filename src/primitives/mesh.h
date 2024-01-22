#pragma once

#include <math/vec.h>
#include <core/vk_types.h>

#include <vector>

namespace vk_primitives {

	namespace mesh {

		struct VertexInputDescription {
			std::vector<VkVertexInputBindingDescription> _bindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;
		};

		struct Vertex_F3_F3 {
			math::Vec3 position;
			math::Vec3 color;

			static VertexInputDescription getVertexInputDescription();
		};

		struct Vertex_F3_F3_F2 {
			math::Vec3 position;
			math::Vec3 normal;
			math::Vec2 texCoords;

			static VertexInputDescription getVertexInputDescription();
		};

	}

}