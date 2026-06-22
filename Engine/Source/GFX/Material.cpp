#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Material.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink::GFX
{
	namespace
	{
		struct TextureSlotDesc {
			std::optional<Resource::TextureHandle> MaterialTextures::*slot;
			const char* uniformName;
			const char* hasUniformName;
			u32 unit;
		};

		constexpr i32 ALBEDO_UNIT = 0;
		constexpr i32 NORMAL_UNIT = 1;

		constexpr TextureSlotDesc TEXTURE_SLOTS[] = {
			{ &MaterialTextures::albedo, "uMaterial.albedoMap", "uMaterial.hasAlbedoMap", ALBEDO_UNIT },
			{ &MaterialTextures::normal, "uMaterial.normalMap", "uMaterial.hasNormalMap", NORMAL_UNIT },
		};
	}

	Material::Material(Resource::ShaderHandle shader,
		MaterialTextures textures, MaterialParams params)
		: shader(shader), textures(textures), params(params)
	{
	}

	void Material::apply() const noexcept
	{
		ShaderProgram* s = Resource::get_shader_pool().try_get(shader);

		s->use();

		const MaterialTextures& tex = textures;

		for (const auto& desc : TEXTURE_SLOTS)
		{
			const auto& optTexID = tex.*(desc.slot);

			if (optTexID.has_value())
			{
				Texture2D* t = Resource::get_texture_pool().try_get(*optTexID);
				t->bind(desc.unit);

				s->set_texture(desc.uniformName, desc.unit);
				s->set(desc.hasUniformName, true);
			}
			else s->set(desc.hasUniformName, false);
		}

		s->set("uMaterial.baseColor", params.baseColor);
	}

	bool Material::is_valid() const noexcept
	{
		return Resource::get_shader_pool().is_valid(shader);
	}
}