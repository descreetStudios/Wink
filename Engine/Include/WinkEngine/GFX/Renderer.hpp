#pragma once

#include <WinkEngine/GFX/Resource/MeshPool.hpp>
#include <WinkEngine/GFX/Resource/ShaderPool.hpp>
#include <WinkEngine/GFX/Resource/TexturePool.hpp>
#include <WinkEngine/GFX/Resource/MaterialPool.hpp>

namespace Wink::GFX
{
	bool init();
	void render();
	void shutdown();

	void resize(u32 width, u32 height);
	void set_clear_color(const glm::vec4& color);

	namespace Resource
	{
		[[nodiscard]] MeshPool& get_mesh_pool() noexcept;
		[[nodiscard]] ShaderPool& get_shader_pool() noexcept;
		[[nodiscard]] TexturePool& get_texture_pool() noexcept;
		[[nodiscard]] MaterialPool& get_material_pool() noexcept;
		void clear_all_resources() noexcept;
	}
}