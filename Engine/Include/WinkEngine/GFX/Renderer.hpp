#pragma once

#include <WinkEngine/GFX/RES/MeshPool.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/RES/TexturePool.hpp>
#include <WinkEngine/GFX/RES/CubemapPool.hpp>
#include <WinkEngine/GFX/RES/MaterialPool.hpp>
#include <WinkEngine/GFX/RES/ModelPool.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline/PostProcessPass.hpp>

namespace Wink::GFX
{
	struct Settings
	{
		Pipeline::PostProcessSettings postProcessSettings;
	};

	bool init(const Settings& settings = {});
	void shutdown();

	void render();
	void resize(u32 width, u32 height);
	void set_clear_color(const glm::vec4& color);

	void apply_settings(const Settings& settings);
	[[nodiscard]] Settings get_settings();

	namespace RES
	{
		[[nodiscard]] MeshPool& get_mesh_pool() noexcept;
		[[nodiscard]] ShaderPool& get_shader_pool() noexcept;
		[[nodiscard]] TexturePool& get_texture_pool() noexcept;
		[[nodiscard]] CubemapPool& get_cubemap_pool()  noexcept;
		[[nodiscard]] MaterialPool& get_material_pool() noexcept;
		[[nodiscard]] ModelPool& get_model_pool() noexcept;

		[[nodiscard]] ShaderHandle get_default_shader() noexcept;
		[[nodiscard]] TextureHandle get_default_albedo() noexcept;
		[[nodiscard]] TextureHandle get_default_normal() noexcept;
		[[nodiscard]] TextureHandle get_default_mr() noexcept;
		[[nodiscard]] TextureHandle get_default_ao() noexcept;
		[[nodiscard]] TextureHandle get_default_emissive() noexcept;
		[[nodiscard]] MaterialHandle get_default_material() noexcept;

		void clear_all_resources() noexcept;
		void poll_hot_reloads() noexcept;
	}

	namespace IBL
	{
		[[nodiscard]] RES::CubemapHandle bake_irradiance_map(
			RES::CubemapHandle envCubemap, u32 faceSize = 32);

		[[nodiscard]] RES::CubemapHandle bake_prefiltered_env_map(
			RES::CubemapHandle envCubemap, u32 faceSize = 256,
			u32 mipLevels = 5, u32 sampleCount = 1024);
	}
}