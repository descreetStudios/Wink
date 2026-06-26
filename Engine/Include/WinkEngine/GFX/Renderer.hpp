#pragma once

#include <WinkEngine/GFX/Resource/MeshPool.hpp>
#include <WinkEngine/GFX/Resource/ShaderPool.hpp>
#include <WinkEngine/GFX/Resource/TexturePool.hpp>
#include <WinkEngine/GFX/Resource/MaterialPool.hpp>
#include <WinkEngine/GFX/Resource/ModelPool.hpp>

namespace Wink::GFX
{
	struct Configuration
	{
		/* --- Depth --- */
		bool depthTest = true;
		bool depthWrite = true;
		u32 depthFunc = GL_LEQUAL;

		/* --- Stencil --- */
		bool stencilTest = false;
		u32 stencilFunc = GL_ALWAYS;
		int stencilRef = 0;
		u32 stencilMask = 0xFF;
		u32 stencilOpSfail = GL_KEEP;
		u32 stencilOpDpfail = GL_KEEP;
		u32 stencilOpDppass = GL_KEEP;

		/* --- Blending --- */
		bool blend = false;
		u32 blendSrc = GL_SRC_ALPHA;
		u32 blendDst = GL_ONE_MINUS_SRC_ALPHA;
		u32 blendEq = GL_FUNC_ADD;

		/* --- Face culling --- */
		bool cullFace = false;
		u32 cullMode = GL_BACK;
		u32 frontFace = GL_CCW;

		/* --- Polygon mode --- */
		u32 polygonMode = GL_FILL;

		/* --- Multisampling --- */
		bool multisample = true;
	};

	struct RenderObject
	{
		Resource::MeshHandle mesh;
		Resource::MaterialHandle material;
	};

	struct CameraData
	{
		glm::vec3 position;
		glm::mat4 viewProj;
	};

#define MAX_DIR_LIGHTS 2
#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8

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

		std::vector<DirLight> dirLights;
		std::vector<PointLight> pointLights;
		std::vector<SpotLight> spotLights;
	};

	bool init();
	void render();
	void shutdown();

	void set_config(const Configuration& cfg);

	void resize(u32 width, u32 height);
	void set_clear_color(const glm::vec4& color);

	namespace Resource
	{
		[[nodiscard]] MeshPool& get_mesh_pool() noexcept;
		[[nodiscard]] ShaderPool& get_shader_pool() noexcept;
		[[nodiscard]] TexturePool& get_texture_pool() noexcept;
		[[nodiscard]] MaterialPool& get_material_pool() noexcept;
		[[nodiscard]] ModelPool& get_model_pool() noexcept;

		[[nodiscard]] ShaderHandle get_default_shader() noexcept;
		[[nodiscard]] MaterialHandle get_default_material() noexcept;
		[[nodiscard]] ShaderHandle get_equirect_to_cubemap_shader() noexcept;

		void clear_all_resources() noexcept;
		void poll_hot_reloads() noexcept;
	}
}