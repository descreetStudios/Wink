#pragma once

#include <WinkEngine/GFX/Resource/Handle.hpp>

namespace Wink::GFX
{
	struct MaterialTextures
	{
		std::optional<Resource::TextureHandle> albedo;
		std::optional<Resource::TextureHandle> normal;
	};

	struct MaterialParams
	{
		glm::vec4 baseColor = glm::vec4(1.0f);
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