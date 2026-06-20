#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/GFX.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
		GFX::set_clear_color({ 0.53f, 0.81f, 0.92f, 1.0f });

		auto& meshPool = GFX::Resource::get_mesh_pool();
		auto& shaderPool = GFX::Resource::get_shader_pool();
		auto& texturePool = GFX::Resource::get_texture_pool();
		auto& materialPool = GFX::Resource::get_material_pool();

		/* Shader resource test */
		auto shaderHandle = shaderPool.load(std::vector<GFX::ShaderFile>{
			{ GFX::ShaderType::Vertex, "Shaders/default_vs.glsl" },
			{ GFX::ShaderType::Fragment, "Shaders/default_fs.glsl" }
		});

		if (shaderPool.is_valid(shaderHandle))
		{
			Logger::info(
				"Successfully loaded shader, ID: '{}'",
				shaderPool.get_id(shaderHandle));
		}

		/* Texture resource test */
		auto textureHandle = texturePool.load("dog.png");

		if (texturePool.is_valid(textureHandle))
		{
			Logger::info(
				"Successfully loaded texture, ID: '{}'",
				texturePool.get_id(textureHandle));
		}

		GFX::MeshData triangleData{
		.vertices = {
			{.position = { -0.5f, -0.5f, 0.0f }, .normal = { 0.0f, 0.0f, 1.0f }, .uv = { 0.0f, 0.0f } },
			{.position = {  0.5f, -0.5f, 0.0f }, .normal = { 0.0f, 0.0f, 1.0f }, .uv = { 1.0f, 0.0f } },
			{.position = {  0.0f,  0.5f, 0.0f }, .normal = { 0.0f, 0.0f, 1.0f }, .uv = { 0.5f, 1.0f } },
		},
		.indices = { 0, 1, 2 }
		};

		/* Mesh resource test */
		auto meshHandle = meshPool.load(triangleData);

		if (meshPool.is_valid(meshHandle))
		{
			Logger::info(
				"Successfully loaded mesh, VAO ID: '{}', index count: '{}'",
				meshPool.get_vao_id(meshHandle),
				meshPool.get_index_count(meshHandle));
		}

		/* Material resource test */
		auto matHandle = materialPool.create(shaderHandle);
		materialPool.set_albedo(matHandle, textureHandle);
		materialPool.set_base_color(matHandle, { 1, 0.5f, 0.5f, 1 });

		auto albedo = materialPool.get_albedo(matHandle);
		if (materialPool.is_valid(matHandle) &&
			albedo.has_value())
		{
			const glm::vec4 color = materialPool.get_base_color(matHandle);

			Logger::info(
				"Successfully created material, albedo ID: '{}', base color: '{}'",
				texturePool.get_id(albedo.value()), glm::to_string(color));
		}
	}
};