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

		auto& shaderPool = GFX::Resource::get_shader_pool();
		auto& texturePool = GFX::Resource::get_texture_pool();

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

		auto textureHandle = texturePool.load("dog.png");

		if (texturePool.is_valid(textureHandle))
		{
			Logger::info(
				"Successfully loaded texture, ID: '{}'",
				texturePool.get_id(textureHandle));
		}
	}
};