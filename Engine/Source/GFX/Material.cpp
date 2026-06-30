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
		constexpr i32 MR_UNIT = 2;
		constexpr i32 AO_UNIT = 3;
		constexpr i32 EMISSIVE_UNIT = 4;

		constexpr TextureSlotDesc TEXTURE_SLOTS[] = {
			{ &MaterialTextures::albedo,            "uMaterial.albedoMap",    "uMaterial.hasAlbedoMap",    ALBEDO_UNIT   },
			{ &MaterialTextures::normal,            "uMaterial.normalMap",    "uMaterial.hasNormalMap",    NORMAL_UNIT   },
			{ &MaterialTextures::metallicRoughness, "uMaterial.mrMap",        "uMaterial.hasMRMap",        MR_UNIT       },
			{ &MaterialTextures::ao,                "uMaterial.aoMap",        "uMaterial.hasAOMap",        AO_UNIT       },
			{ &MaterialTextures::emissive,          "uMaterial.emissiveMap",  "uMaterial.hasEmissiveMap",  EMISSIVE_UNIT },
		};
	}

	Material::Material(Resource::ShaderHandle shader,
		MaterialTextures textures, MaterialParams params)
		: textures(textures), params(params), shader(shader)
	{
	}

	void Material::apply() const noexcept
	{
		assert(is_valid());

		ShaderProgram* s = Resource::get_shader_pool().try_get(shader);
		assert(s);

		s->use();

		for (const auto& desc : TEXTURE_SLOTS)
		{
			const auto& optTexID = textures.*(desc.slot);

			if (optTexID.has_value())
			{
				Texture2D* t = Resource::get_texture_pool().try_get(*optTexID);
				assert(t);

				t->bind(desc.unit);

				s->set_texture(desc.uniformName, desc.unit);
				s->set(desc.hasUniformName, true);
			}
			else s->set(desc.hasUniformName, false);
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
		return Resource::get_shader_pool().is_valid(shader);
	}
}