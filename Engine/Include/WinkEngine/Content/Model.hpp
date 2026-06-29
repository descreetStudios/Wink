#pragma once

#include <WinkEngine/GFX/Resource/Handle.hpp>

namespace Wink::Content
{
	struct ModelPrimitive
	{
		GFX::Resource::MeshHandle mesh;
		GFX::Resource::MaterialHandle material;
	};

	struct ModelNode
	{
		std::string name;
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::identity<glm::quat>();
		glm::vec3 scale = glm::vec3(1.0f);

		std::optional<u32> parent;
		std::vector<ModelPrimitive> primitives;
	};

	struct Model
	{
		std::vector<ModelNode> nodes;
	};

	namespace Internal
	{
		const std::vector<std::string> SUPPORTED_MODEL_FORMATS{
			".gltf",
			".glb"
		};

		std::optional<Model> load_gltf(
			const fs::path& path, 
			GFX::Resource::ShaderHandle shader = {});
	}
}