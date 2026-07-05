#pragma once

#include <WinkEngine/GFX/RES/Handle.hpp>

namespace Wink::GFX
{
	struct MaterialTextures
	{
		RES::TextureHandle albedo;
		RES::TextureHandle normal;
		RES::TextureHandle mr;
		RES::TextureHandle ao;
		RES::TextureHandle emissive;
	};

	struct MaterialTextureHandles
	{
		u64 albedo = 0;
		u64 normal = 0;
		u64 mr = 0;
		u64 ao = 0;
		u64 emissive = 0;
		u64 _pad = 0;
	};

	struct MaterialParams
	{
		glm::vec4 baseColor = glm::vec4(1.0f);
		float metallic = 0.0f;
		float roughness = 1.0f;
		float aoStrength = 1.0f;
		float _pad;
		glm::vec4 emissiveFactor = glm::vec4(0.0f); // .w unused

		u32 albedoTexCoord = 0;
		u32 normalTexCoord = 0;
		u32 mrTexCoord = 0;
		u32 aoTexCoord = 0;
		u32 emissiveTexCoord = 0;
		u32 _pad1[3] = {};
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

		static void init_ubo();
		static void destroy_ubo();
		void apply() const noexcept;

		[[nodiscard]] bool is_valid() const noexcept;

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		void set_default_textures() noexcept;
	};
}