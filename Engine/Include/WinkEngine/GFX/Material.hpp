#pragma once

#include <WinkEngine/GFX/Resource/Handle.hpp>

namespace Wink::GFX
{
	struct MaterialTextures
	{
		std::optional<Resource::TextureHandle> albedo;
		std::optional<Resource::TextureHandle> normal;
		std::optional<Resource::TextureHandle> metallicRoughness;
		std::optional<Resource::TextureHandle> ao;
		std::optional<Resource::TextureHandle> emissive;
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
		Resource::ShaderHandle shader;

	public:
		Material() = default;
		explicit Material(Resource::ShaderHandle shader,
			MaterialTextures textures = {},
			MaterialParams params = {});

		void apply() const noexcept;

		[[nodiscard]] bool is_valid() const noexcept;

		explicit operator bool() const noexcept { return is_valid(); }
	};
}