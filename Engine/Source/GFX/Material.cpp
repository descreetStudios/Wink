#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Material.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink::GFX
{
	namespace
	{
		constexpr i32 ALBEDO_UNIT = 0;
		constexpr i32 NORMAL_UNIT = 1;
	}

	Material::Material(Resource::ShaderHandle shader,
		MaterialTextures textures, MaterialParams params)
		: shader(shader), textures(textures), params(params)
	{
	}

	void Material::apply() const noexcept
	{
		auto& shaderPool = Resource::get_shader_pool();
		auto& texturePool = Resource::get_texture_pool();

		shaderPool.use(shader);

		if (textures.albedo.has_value())
		{
			texturePool.bind(*textures.albedo, ALBEDO_UNIT);
			shaderPool.set_texture(shader, "uAlbedoMap", ALBEDO_UNIT);
			shaderPool.set(shader, "uHasAlbedoMap", true);
		}
		else shaderPool.set(shader, "uHasAlbedoMap", false);

		if (textures.normal.has_value())
		{
			texturePool.bind(*textures.normal, NORMAL_UNIT);
			shaderPool.set_texture(shader, "uNormalMap", NORMAL_UNIT);
			shaderPool.set(shader, "uHasNormalMap", true);
		}
		else shaderPool.set(shader, "uHasNormalMap", false);

		shaderPool.set(shader, "uBaseColor", params.baseColor);
	}

	bool Material::is_valid() const noexcept
	{
		auto& shaderPool = Resource::get_shader_pool();
		auto& texturePool = Resource::get_texture_pool();

		return shaderPool.is_valid(shader) &&
			textures.albedo.has_value() && texturePool.is_valid(*textures.albedo) &&
			textures.normal.has_value() && texturePool.is_valid(*textures.normal);
	}
}