#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/GFX.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
		using namespace GFX;
		using namespace Resource;

		set_clear_color({ 0.53f, 0.81f, 0.92f, 1.0f });

		auto& meshPool = Resource::get_mesh_pool();
		auto& shaderPool = Resource::get_shader_pool();
		auto& texturePool = Resource::get_texture_pool();
		auto& materialPool = Resource::get_material_pool();

		/* --- Shader --- */
		const ShaderHandle shader = shaderPool.load(std::vector<ShaderFile>{
			{ ShaderType::Vertex, "Shaders/default_vs.glsl" },
			{ ShaderType::Fragment, "Shaders/default_fs.glsl" }
		});

		if (shader)
			Logger::info("Loaded shader ID '{}'", shaderPool.get_id(shader));

		// --- Texture ---
		const TextureHandle tex = texturePool.load("dog.png");

		if (tex)
			Logger::info("Loaded texture ID '{}'", texturePool.get_id(tex));

		// --- Mesh ---
		const MeshData triangle{
			.vertices = {
				{ { -0.5f, -0.5f, 0.0f }, { 0,0,1 }, { 0,0 } },
				{ {  0.5f, -0.5f, 0.0f }, { 0,0,1 }, { 1,0 } },
				{ {  0.0f,  0.5f, 0.0f }, { 0,0,1 }, { 0.5f,1 } },
			},
			.indices = { 0, 1, 2 }
		};

		const MeshHandle mesh = meshPool.load(triangle);

		if (mesh)
			Logger::info("Loaded mesh VAO '{}', index count '{}'",
				meshPool.get_vao_id(mesh),
				meshPool.get_index_count(mesh));

		// --- Material ---
		const MaterialHandle mat = materialPool.create(shader);

		if (auto* m = materialPool.try_get(mat))
		{
			m->params.baseColor = { 1.0f, 0.5f, 0.2f, 1.0f };
			m->textures.albedo = tex;

			Logger::info("Created material, albedo '{}', baseColor '{}'",
				texturePool.get_id(*m->textures.albedo),
				glm::to_string(m->params.baseColor));
		}
	}
};