#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/ECS.hpp>
#include <WinkEngine/GFX.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
		using namespace GFX;

		auto* scene = ECS::create_scene("Resources Test");

		auto& meshPool = RES::get_mesh_pool();
		auto& shaderPool = RES::get_shader_pool();
		auto& texturePool = RES::get_texture_pool();
		auto& materialPool = RES::get_material_pool();
		auto& modelPool = RES::get_model_pool();

		/* --- Shader --- */
		const RES::ShaderHandle shader = shaderPool.load(std::vector<ShaderFile>{
			{ ShaderType::Vertex, "Resources/Shaders/DefaultVS.glsl" },
			{ ShaderType::Fragment, "Resources/Shaders/DefaultFS.glsl" }
		});

		if (shader)
			Logger::info("Loaded shader ID '{}'", shaderPool.get_id(shader));

		/* --- Texture --- */
		const RES::TextureHandle tex = texturePool.decode("dog.png");

		if (tex)
			Logger::info("Loaded texture ID '{}'", texturePool.get_id(tex));

		/* --- Mesh --- */
		const MeshData triangle{
			.vertices = {
				{ { -0.5f, -0.5f, 0.0f }, { 0,0,1 }, { 0,0 } },
				{ {  0.5f, -0.5f, 0.0f }, { 0,0,1 }, { 1,0 } },
				{ {  0.0f,  0.5f, 0.0f }, { 0,0,1 }, { 0.5f,1 } },
			},
			.indices = { 0, 1, 2 }
		};

		const RES::MeshHandle mesh = meshPool.load(triangle);

		if (mesh)
			Logger::info("Loaded mesh VAO '{}', index count '{}'",
				meshPool.get_vao_id(mesh),
				meshPool.get_index_count(mesh));

		/* --- Material --- */
		const RES::MaterialHandle mat = materialPool.create(shader);

		if (auto* m = materialPool.try_get(mat))
		{
			m->params.baseColor = { 1.0f, 0.5f, 0.2f, 1.0f };
			m->textures.albedo = tex;

			Logger::info("Created material, albedo '{}', baseColor '{}'",
				texturePool.get_id(*m->textures.albedo),
				glm::to_string(m->params.baseColor));
		}

		/* --- Model --- */
		fs::path root = fs::path("..") / ".." / ".." / "..";
		fs::path models = root / fs::path("..") / "glTF-Sample-Models" / "2.0";
		const RES::ModelHandle sponza = modelPool.load(models / "Sponza" / "glTF");

		if (auto* m = modelPool.try_get(sponza))
		{
			Logger::info("Model loaded");
			Logger::info("Nodes: {}", m->nodes.size());

			for (u32 i = 0; i < m->nodes.size(); i++)
			{
				const auto& node = m->nodes[i];

				Logger::info("Node [{}]: {}", i, node.name);

				Logger::info(
					"  Position: {}, {}, {}",
					node.position.x,
					node.position.y,
					node.position.z
				);

				Logger::info(
					"  Rotation: {}, {}, {}, {}",
					node.rotation.x,
					node.rotation.y,
					node.rotation.z,
					node.rotation.w
				);

				Logger::info(
					"  Scale: {}, {}, {}",
					node.scale.x,
					node.scale.y,
					node.scale.z
				);

				if (node.parent.has_value())
					Logger::info("  Parent: {}", node.parent.value());
				else
					Logger::info("  Parent: none");

				Logger::info("  Primitives: {}", node.primitives.size());

				for (u32 j = 0; j < node.primitives.size(); j++)
				{
					const auto& primitive = node.primitives[j];

					Logger::info("    Primitive [{}]", j);

					Logger::info("      Mesh valid: {}", primitive.mesh.is_valid());
					Logger::info("      Material valid: {}", primitive.material.is_valid());
				}
			}
		}
	}
};