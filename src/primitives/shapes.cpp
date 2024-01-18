#include "shapes.h"

std::vector<vk_primitives::mesh::Vertex_F3_F3> vk_primitives::shapes::Cube::getVertexData()
{
	return {
		//Front vertices
		{ {0.5f, 0.5f, 0.5}, { 0.0f,0.0f,0.0f }}, //Front-BottomRight (0)
		{ {-0.5f,0.5f,0.5}, {1.0f,1.0f,1.0f} }, //Front-BottomLeft (1)
		{ {-0.5f,-0.5f,0.5}, {1.0f,0.0f,0.0f} }, //Front-TopLeft (2)
		{ {0.5f,-0.5f,0.5}, {0.0f,1.0f,0.0f} }, //Front-TopRight (3)

		//Back vertices
		{ {0.5f, 0.5f, -0.5}, { 0.0f,0.0f,0.0f }}, //Back-BottomRight (4)
		{ {-0.5f,0.5f,-0.5}, {1.0f,1.0f,1.0f} }, //Back-BottomLeft (5)
		{ {-0.5f,-0.5f,-0.5}, {1.0f,0.0f,0.0f} }, //Back-TopLeft (6)
		{ {0.5f,-0.5f,-0.5}, {0.0f,1.0f,0.0f} }, //Back-TopRight (7)
	};
}

std::vector<uint32_t> vk_primitives::shapes::Cube::getIndexData()
{
	return {
		//Front Face
		0,2,3,0,1,2,

		//Left Face
		1,6,2,1,5,6,
		
		//Back Face
		5,7,6,5,4,7,

		//Right Face
		4,3,7,4,0,3,

		//Top Face
		3,6,7,3,2,6,

		//Bottom Face
		4,1,0,4,5,1
	};
}