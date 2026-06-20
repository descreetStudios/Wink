#pragma once

#include <WinkEngine/GFX/VAO.hpp>
#include <WinkEngine/GFX/Buffer.hpp>

namespace Wink::GFX
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct MeshData
	{
		std::vector<Vertex> vertices;
		std::vector<u32> indices;
	};

	struct Mesh
	{
		VAO vao;
		Buffer vbo{ BufferTarget::Vertex };
		Buffer ebo{ BufferTarget::Index };
		u32 indexCount = 0;
	};
}