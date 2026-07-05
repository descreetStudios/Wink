#pragma once

#include <WinkEngine/GFX/RES/MeshPool.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/RES/TexturePool.hpp>
#include <WinkEngine/GFX/RES/CubemapPool.hpp>
#include <WinkEngine/GFX/RES/MaterialPool.hpp>
#include <WinkEngine/GFX/RES/ModelPool.hpp>

namespace Wink::GFX
{
	bool init();
	void shutdown();

	void render();
	void resize(u32 width, u32 height);
	void set_clear_color(const glm::vec4& color);

	struct Configuration
	{
		/* --- Depth --- */
		u32 depthFunc = GL_LEQUAL;
		bool depthTest = true;
		bool depthWrite = true;

		/* --- Multisampling --- */
		bool multisample = true;

		/* --- Stencil --- */
		bool stencilTest = false;
		u32 stencilFunc = GL_ALWAYS;
		i32 stencilRef = 0;
		u32 stencilMask = 0xFF;
		u32 stencilOpSfail = GL_KEEP;
		u32 stencilOpDpfail = GL_KEEP;
		u32 stencilOpDppass = GL_KEEP;

		/* --- Face culling --- */
		u32 cullMode = GL_BACK;
		u32 frontFace = GL_CCW;
		bool cullFace = false;

		/* --- Blending --- */
		bool blend = false;
		u32 blendSrc = GL_SRC_ALPHA;
		u32 blendDst = GL_ONE_MINUS_SRC_ALPHA;
		u32 blendEq = GL_FUNC_ADD;

		/* --- Polygon mode --- */
		u32 polygonMode = GL_FILL;
	};

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