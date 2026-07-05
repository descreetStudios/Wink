#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Material.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink::GFX
{
	namespace
	{
		constexpr u32 MATERIAL_SSBO_BINDING = 2;
		u32 gMaterialSSBO = 0;

		// Full buffer layout — what gets uploaded each draw
		struct MaterialBufferData
		{
			MaterialTextureHandles handles;
			MaterialParams params;
		};

		static_assert(sizeof(MaterialBufferData) % 16 == 0);
	}

	void Material::init_ssbo()
	{
		glCreateBuffers(1, &gMaterialSSBO);
		glNamedBufferStorage(gMaterialSSBO,
			sizeof(MaterialBufferData),
			nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	void Material::destroy_ssbo()
	{
		glDeleteBuffers(1, &gMaterialSSBO);
		gMaterialSSBO = 0;
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
		fill(textures.mr, GFX::RES::get_default_mr());
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
		assert(gMaterialSSBO != 0);

		ShaderProgram* s = RES::get_shader_pool().try_get(shader);
		assert(s);
		s->use();

		auto resolve = [](RES::TextureHandle h) -> u64
			{
				const Texture2D* t = RES::get_texture_pool().try_get(h);
				assert(t && t->is_resident());
				return t->get_bindless_handle();
			};

		const MaterialBufferData data
		{
			.handles =
			{
				.albedo = resolve(textures.albedo),
				.normal = resolve(textures.normal),
				.mr = resolve(textures.mr),
				.ao = resolve(textures.ao),
				.emissive = resolve(textures.emissive),
			},
			.params = params,
		};

		glNamedBufferSubData(gMaterialSSBO, 0, sizeof(data), &data);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
			MATERIAL_SSBO_BINDING, gMaterialSSBO);
	}

	bool Material::is_valid() const noexcept
	{
		return RES::get_shader_pool().is_valid(shader);
	}
}