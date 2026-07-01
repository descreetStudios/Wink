#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Material.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink::GFX
{
	namespace
	{
		struct TextureSlotDesc
		{
			RES::TextureHandle MaterialTextures::*slot;
			const char* uniformName;
			i32 unit;
		};

		constexpr i32 ALBEDO_UNIT = 0;
		constexpr i32 NORMAL_UNIT = 1;
		constexpr i32 MR_UNIT = 2;
		constexpr i32 AO_UNIT = 3;
		constexpr i32 EMISSIVE_UNIT = 4;

		constexpr TextureSlotDesc TEXTURE_SLOTS[]{
			{ &MaterialTextures::albedo, "uMaterial.albedoMap", ALBEDO_UNIT },
			{ &MaterialTextures::normal, "uMaterial.normalMap", NORMAL_UNIT },
			{ &MaterialTextures::metallicRoughness, "uMaterial.mrMap", MR_UNIT },
			{ &MaterialTextures::ao, "uMaterial.aoMap", AO_UNIT },
			{ &MaterialTextures::emissive, "uMaterial.emissiveMap", EMISSIVE_UNIT },
		};
	}

	void Material::set_default_textures() noexcept
	{
		auto fill = [](RES::TextureHandle& slot, RES::TextureHandle fallback)
			{
				if (!RES::get_texture_pool().is_valid(slot))
					slot = fallback;
			};

		fill(textures.albedo, GFX::RES::get_default_albedo());
		fill(textures.normal, GFX::RES::get_default_normal());
		fill(textures.metallicRoughness, GFX::RES::get_default_mr());
		fill(textures.ao, GFX::RES::get_default_ao());
		fill(textures.emissive, GFX::RES::get_default_emissive());
	}

	Material::Material()
	{
		set_default_textures();
	}

	Material::Material(RES::ShaderHandle shader,
		MaterialTextures textures, MaterialParams params)
		: textures(textures), params(params), shader(shader)
	{
		set_default_textures();
	}

	void Material::apply() const noexcept
	{
		assert(is_valid());

		ShaderProgram* s = RES::get_shader_pool().try_get(shader);
		assert(s);

		s->use();

		for (const auto& desc : TEXTURE_SLOTS)
		{
			const RES::TextureHandle handle = textures.*(desc.slot);

			Texture2D* t = RES::get_texture_pool().try_get(handle);
			assert(t);

			t->bind(desc.unit);
			s->set(desc.uniformName, desc.unit);
		}

		s->set("uMaterial.baseColor", params.baseColor);
		s->set("uMaterial.metallic", params.metallic);
		s->set("uMaterial.roughness", params.roughness);
		s->set("uMaterial.emissiveFactor", params.emissiveFactor);
		s->set("uMaterial.aoStrength", params.aoStrength);

		s->set("uMaterial.albedoTexCoord", static_cast<i32>(params.albedoTexCoord));
		s->set("uMaterial.normalTexCoord", static_cast<i32>(params.normalTexCoord));
		s->set("uMaterial.mrTexCoord", static_cast<i32>(params.mrTexCoord));
		s->set("uMaterial.aoTexCoord", static_cast<i32>(params.aoTexCoord));
		s->set("uMaterial.emissiveTexCoord", static_cast<i32>(params.emissiveTexCoord));
	}

	bool Material::is_valid() const noexcept
	{
		return RES::get_shader_pool().is_valid(shader);
	}
}