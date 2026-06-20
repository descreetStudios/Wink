#pragma once

#include <WinkEngine/GFX/Resource/ShaderPool.hpp>
#include <WinkEngine/GFX/Resource/TexturePool.hpp>

namespace Wink::GFX
{
	bool init();
	void render();
	void shutdown();

	void resize(u32 width, u32 height);
	void set_clear_color(const glm::vec4& color);

	namespace Resource
	{
		[[nodiscard]] ShaderPool& get_shader_pool() noexcept;
		[[nodiscard]] TexturePool& get_texture_pool() noexcept;
		void clear_all_resources() noexcept;
	}
}