#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/GFX.hpp>
#include <WinkEngine/Resource.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
		GFX::set_clear_color({ 0.53f, 0.81f, 0.92f, 1.0f });

		auto& shaderPool = GFX::get_shader_pool();

		auto h = shaderPool.load(std::vector<GFX::ShaderFile>{
			{ GFX::ShaderType::Vertex, "Shaders/default_vs.glsl" },
			{ GFX::ShaderType::Fragment, "Shaders/default_fs.glsl" }
		});

		if (shaderPool.is_valid(h))
		{
			Logger::info(
				"Successfully loaded shader, ID: '{}'",
				shaderPool.get_id(h));
		}
	}
};