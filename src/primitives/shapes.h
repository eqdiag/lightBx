#pragma once

#include "mesh.h"

#include <vector>

namespace vk_primitives {

	namespace shapes {

		class Cube {
		public:
			static std::vector<mesh::Vertex_F3_F3> getVertexData();
			static std::vector<uint32_t> getIndexData();
		};

	}
}