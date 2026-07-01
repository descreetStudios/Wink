#pragma once

#include <WinkEngine/GFX/RES/Handle.hpp>

namespace Wink::GFX
{
	struct MaterialTextures
	{
		RES::TextureHandle albedo;
		RES::TextureHandle normal;
		RES::TextureHandle metallicRoughness;
		RES::TextureHandle ao;
		RES::TextureHandle emissive;
	};

	struct MaterialParams
	{
		glm::vec4 baseColor = glm::vec4(1.0f);
		float metallic = 0.0f;
		float roughness = 1.0f;
		glm::vec3 emissiveFactor = glm::vec3(0.0f);
		float aoStrength = 1.0f;

		u32 albedoTexCoord = 0;
		u32 normalTexCoord = 0;
		u32 mrTexCoord = 0;
		u32 aoTexCoord = 0;
		u32 emissiveTexCoord = 0;
	};

	class Material
	{
	public:
		MaterialTextures textures;
		MaterialParams params;
		RES::ShaderHandle shader;

	public:
		Material();
		explicit Material(RES::ShaderHandle shader,
			MaterialTextures textures = {},
			MaterialParams params = {});

		void apply() const noexcept;

		[[nodiscard]] bool is_valid() const noexcept;

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		void set_default_textures() noexcept;
	};
}