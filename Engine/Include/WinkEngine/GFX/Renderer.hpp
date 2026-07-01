#pragma once

#include <WinkEngine/GFX/RES/MeshPool.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/RES/TexturePool.hpp>
#include <WinkEngine/GFX/RES/CubemapPool.hpp>
#include <WinkEngine/GFX/RES/MaterialPool.hpp>
#include <WinkEngine/GFX/RES/ModelPool.hpp>

namespace Wink::GFX
{
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

	struct RenderObject
	{
		RES::MeshHandle mesh;
		RES::MaterialHandle material;
	};

	struct CameraData
	{
		glm::vec3 position;
		glm::mat4 viewProj;
	};

	inline constexpr u32 MAX_DIR_LIGHTS = 2;
	inline constexpr u32 MAX_POINT_LIGHTS = 16;
	inline constexpr u32 MAX_SPOT_LIGHTS = 8;

	struct DirLight
	{
		glm::vec3 direction = { 0.0f, -1.0f, 0.0f };
		float intensity = 1.0f;
		glm::vec3 color = glm::vec3(1.0f);
	};

	struct PointLight
	{
		glm::vec3 position = glm::vec3(0.0f);
		float intensity = 1.0f;
		glm::vec3 color = glm::vec3(1.0f);
		float radius = 15.0f;
	};

	struct SpotLight
	{
		glm::vec3 position = glm::vec3(0.0f);
		float range = 45.0f;
		glm::vec3 direction = { 0.0f, -1.0f, 0.0f };
		float innerCutoff = glm::cos(glm::radians(15.0f));
		glm::vec3 color = glm::vec3(1.0f);
		float outerCutoff = glm::cos(glm::radians(30.0f));
		float intensity = 1.0f;
	};

	struct DrawData
	{
		const RenderObject& renderObj;
		const CameraData& camData;
		const glm::mat4& modelMat;
		const glm::mat3& normalMat;

		std::span<const DirLight> dirLights;
		std::span<const PointLight> pointLights;
		std::span<const SpotLight> spotLights;
	};

	bool init();
	void render();
	void render_fullscreen_texture(RES::TextureHandle tex);
	void shutdown();

	void set_config(const Configuration& cfg);

	void resize(u32 width, u32 height);
	void set_clear_color(const glm::vec4& color);

	namespace RES
	{
		[[nodiscard]] MeshPool& get_mesh_pool() noexcept;
		[[nodiscard]] ShaderPool& get_shader_pool() noexcept;
		[[nodiscard]] TexturePool& get_texture_pool() noexcept;
		[[nodiscard]] CubemapPool& get_cubemap_pool()  noexcept;
		[[nodiscard]] MaterialPool& get_material_pool() noexcept;
		[[nodiscard]] ModelPool& get_model_pool() noexcept;

		[[nodiscard]] ShaderHandle get_default_shader() noexcept;
		[[nodiscard]] MaterialHandle get_default_material() noexcept;

		void clear_all_resources() noexcept;
		void poll_hot_reloads() noexcept;
	}

	namespace IBL
	{
		struct IBLData
		{
			RES::CubemapHandle envMap;
			RES::CubemapHandle irradianceMap;
			RES::CubemapHandle prefilteredEnvMap;
		};

		[[nodiscard]] RES::CubemapHandle bake_irradiance_map(
			RES::CubemapHandle envCubemap, u32 faceSize = 32);

		[[nodiscard]] RES::CubemapHandle bake_prefiltered_env_map(
			RES::CubemapHandle envCubemap, u32 faceSize = 256,
			u32 mipLevels = 5, u32 sampleCount = 1024);

		namespace Internal
		{
			[[nodiscard]] RES::CubemapHandle equirect_to_cubemap(
				RES::TextureHandle hdr, u32 faceSize = 512);
		}
	}
}