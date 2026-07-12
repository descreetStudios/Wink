#pragma once

#include <WinkEngine/GFX/Framebuffer.hpp>
#include <WinkEngine/GFX/Texture2DArray.hpp>
#include <WinkEngine/GFX/Renderer/Data.hpp>
#include <WinkEngine/GFX/Renderer/GPUData.hpp>

namespace Wink::GFX::Pipeline
{
	struct CSMSettings
	{
		float zNear = 0.1f;
		float zFar = 100.0f;
		float lambda = 0.75f;
		float lightOrthoZ = 250.0f;
	};

	class ShadowPass
	{
	public:
		~ShadowPass() noexcept;

		bool init() noexcept;
		void execute(
			const DirLight& light,
			std::span<const RenderObject> objects,
			std::span<const glm::mat4> modelMats,
			const glm::mat4& cameraView,
			const glm::mat4& cameraProj,
			const CSMSettings& settings = {}) noexcept;

		void bind_shadow_map(u32 unit) const noexcept;
		void debug_draw(u32 width, u32 height, u32 cascade) noexcept;

	private:
		void compute_splits(float zNear, float zFar, float lambda) noexcept;

		void compute_stable_texel_sizes(
			float zNear, float zFar, float lambda,
			float fovY, float aspect) noexcept;

		void compute_cascade_matrix(u32 cascade,
			const glm::vec3& lightDir, const glm::mat4& lightView,
			const glm::mat4& invVP, const glm::vec3& cameraPos,
			float zNear, float zFar, float lightOrthoZ,
			float stableTexelSize) noexcept;

	private:
		Framebuffer mFBO;
		Texture2DArray mShadowMap;
		u32 mSamplerCmp = 0;
		u32 mSamplerRaw = 0;
		u32 mShadowUBO = 0;

		ShadowGPUData mUBOData{};

		std::array<float, NUM_CASCADES> mSplitDepths{};
		std::array<glm::mat4, NUM_CASCADES> mLightSpaceMatrices{};
		std::array<float, NUM_CASCADES> mCascadeOrthoSizes{};
		std::array<float, NUM_CASCADES> mStableTexelSizes{};
	};
}