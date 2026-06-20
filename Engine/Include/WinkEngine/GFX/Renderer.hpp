#pragma once

#include <WinkEngine/Resource/ShaderPool.hpp>

namespace Wink::GFX
{
	bool init();
	void render();
	void shutdown();

	void resize(u32 width, u32 height);
	void set_clear_color(const glm::vec4& color);

	/* --- Resources --- */
	[[nodiscard]] Resource::ShaderPool& get_shader_pool() noexcept;
	void clear_all_resources() noexcept;
}