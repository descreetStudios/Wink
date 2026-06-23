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
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct DrawData
	{
		const RenderObject& renderObj;
		const CameraData& camData;
		const glm::mat4& modelMat;
	};

	bool init();
	void render(const Configuration& cfg = {});
	void shutdown();

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

		void clear_all_resources() noexcept;
		void poll_hot_reloads() noexcept;
	}
}