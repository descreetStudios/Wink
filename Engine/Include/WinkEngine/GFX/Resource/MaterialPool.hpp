#pragma once

#include <WinkEngine/GFX/Resource/ResourcePool.hpp>
#include <WinkEngine/GFX/Material.hpp>

namespace Wink::GFX::Resource
{
	class MaterialPool final : public ResourcePool<Material, MaterialTag>
	{
	public:
		MaterialHandle create(ShaderHandle shader);

		void destroy(MaterialHandle handle) noexcept;

		/* Material mutations */
		void set_shader(MaterialHandle handle, ShaderHandle shader) const noexcept;

		void set_params(MaterialHandle handle, MaterialParams params) const noexcept;
		void set_base_color(MaterialHandle handle, glm::vec4 color) const noexcept;

		void set_textures(MaterialHandle handle, MaterialTextures textures) const noexcept;
		void set_albedo(MaterialHandle handle, std::optional<TextureHandle> tex) const noexcept;
		void set_normal(MaterialHandle handle, std::optional<TextureHandle> tex) const noexcept;

		/* Material operations */
		void apply(MaterialHandle handle) const noexcept;

		[[nodiscard]] ShaderHandle get_shader(MaterialHandle handle) const noexcept;

		[[nodiscard]] MaterialTextures get_textures(MaterialHandle handle) const noexcept;
		[[nodiscard]] std::optional<TextureHandle> get_albedo(MaterialHandle handle) const noexcept;

		[[nodiscard]] MaterialParams get_params(MaterialHandle handle) const noexcept;
		[[nodiscard]] glm::vec4 get_base_color(MaterialHandle handle) const noexcept;

		[[nodiscard]] bool is_valid(MaterialHandle handle) const noexcept;
	};
}