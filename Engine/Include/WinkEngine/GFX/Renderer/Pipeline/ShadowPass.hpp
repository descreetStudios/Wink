#pragma once

#include <WinkEngine/GFX/Framebuffer.hpp>
#include <WinkEngine/GFX/Texture2D.hpp>
#include <WinkEngine/GFX/Renderer/Data.hpp>

namespace Wink::GFX::Pipeline
{
	inline constexpr u32 SHADOW_MAP_SIZE = 4096;

	struct ShadowOrthoSettings
	{
		float left = -20.0f;
		float right = 20.0f;
		float bottom = -20.0f;
		float top = 20.0f;
		float zNear = 0.1f;
		float zFar = 200.0f;
	};

	class ShadowPass
	{
	public:
		bool init() noexcept;

		void execute(
			const DirLight& light,
			std::span<const RenderObject> objects,
			std::span<const glm::mat4> modelMats,
			const ShadowOrthoSettings& ortho = {}) noexcept;

		void bind_shadow_map(RES::ShaderHandle shader, u32 unit) const noexcept;
		void debug_draw(u32 width, u32 height) noexcept;

		[[nodiscard]] const glm::mat4& get_light_space_matrix() const noexcept { return mLightSpaceMatrix; }
		[[nodiscard]] u32 get_shadow_map_id() const noexcept { return mShadowMap.get_id(); }

	private:
		Framebuffer mFBO;
		Texture2D mShadowMap;
		glm::mat4 mLightSpaceMatrix = glm::mat4(1.0f);
	};
}