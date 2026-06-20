#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Resource/MaterialPool.hpp>

namespace Wink::GFX::Resource
{
	MaterialHandle MaterialPool::create(ShaderHandle shader)
	{
		return allocate(shader);
	}

	void MaterialPool::destroy(MaterialHandle handle) noexcept
	{
		deallocate(handle);
	}

	void MaterialPool::set_shader(
		MaterialHandle handle, ShaderHandle shader) const noexcept
	{
		with(handle, [&](Material& m) { m.shader = shader; });
	}

	void MaterialPool::set_params(
		MaterialHandle handle, MaterialParams params) const noexcept
	{
		with(handle, [&](Material& m) { m.params = params; });
	}

	void MaterialPool::set_base_color(
		MaterialHandle handle, glm::vec4 color) const noexcept
	{
		with(handle, [&](Material& m) { m.params.baseColor = color; });
	}

	void MaterialPool::set_textures(
		MaterialHandle handle, MaterialTextures textures) const noexcept
	{
		with(handle, [&](Material& m) { m.textures = textures; });
	}

	void MaterialPool::set_albedo(MaterialHandle handle,
		std::optional<TextureHandle> tex) const noexcept
	{
		with(handle, [&](Material& m) { m.textures.albedo = tex; });
	}

	void MaterialPool::set_normal(MaterialHandle handle,
		std::optional<TextureHandle> tex) const noexcept
	{
		with(handle, [&](Material& m) { m.textures.normal = tex; });
	}

	void MaterialPool::apply(MaterialHandle handle) const noexcept
	{
		with(handle, [](Material& m) { m.apply(); });
	}

	ShaderHandle MaterialPool::get_shader(
		MaterialHandle handle) const noexcept
	{
		ShaderHandle shader{};
		with(handle, [&](Material& m) { shader = m.shader; });
		return shader;
	}

	MaterialTextures MaterialPool::get_textures(
		MaterialHandle handle) const noexcept
	{
		MaterialTextures textures;
		with(handle, [&](Material& m) { textures = m.textures; });
		return textures;
	}

	std::optional<TextureHandle> MaterialPool::get_albedo(
		MaterialHandle handle) const noexcept
	{
		std::optional<TextureHandle> tex;
		with(handle, [&](Material& m) { tex = m.textures.albedo; });
		return tex;
	}

	MaterialParams MaterialPool::get_params(MaterialHandle handle) const noexcept
	{
		MaterialParams params;
		with(handle, [&](Material& m) { params = m.params; });
		return params;
	}

	glm::vec4 MaterialPool::get_base_color(
		MaterialHandle handle) const noexcept
	{
		glm::vec4 color(1.0f);
		with(handle, [&](Material& m) { color = m.params.baseColor; });
		return color;
	}

	bool MaterialPool::is_valid(MaterialHandle handle) const noexcept
	{
		bool valid = ResourcePool::is_valid(handle);
		with(handle, [&](Material& m) { valid &= m.is_valid(); });
		return valid;
	}
}