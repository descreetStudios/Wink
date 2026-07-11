#pragma once

#include <WinkEngine/GFX/Framebuffer.hpp>
#include <WinkEngine/GFX/Texture2DArray.hpp>
#include <WinkEngine/GFX/Renderer/Data.hpp>

namespace Wink::GFX::Pipeline
{
	static constexpr u32 SHADOW_MAP_SIZE = 4096;
	static constexpr u32 NUM_CASCADES = 4;

	struct CSMSettings
	{
		float zNear = 0.1f;
		float zFar = 100.0f;
		float lambda = 0.75f; // 0 = linear splits, 1 = logarithmic
		float lightOrthoZ = 250.0f;
		float maxShadowDist = 100.0f;
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

		void bind_shadow_map(RES::ShaderHandle shader, u32 unit) const noexcept;
		void debug_draw(u32 width, u32 height, u32 cascade) noexcept;

		const std::array<float, NUM_CASCADES>& 
			get_split_depths() const noexcept { return mSplitDepths; }
		const std::array<glm::mat4, NUM_CASCADES>& 
			get_light_space_matrices() const noexcept { return mLightSpaceMatrices; }

	private:
		void compute_splits(float zNear, float zFar, float lambda) noexcept;

		void compute_stable_texel_sizes(
			float zNear, float zFar, float lambda,
			float fovY, float aspect) noexcept;

		void compute_cascade_matrix(
			u32 cascade,
			const glm::vec3& lightDir,
			const glm::mat4& cameraView,
			const glm::mat4& cameraProj,
			float zNear, float zFar,
			float lightOrthoZ,
			float stableTexelSize) noexcept;

	private:
		Framebuffer mFBO;
		Texture2DArray mShadowMap;
		u32 mSamplerCmp = 0;
		u32 mSamplerRaw = 0;

		std::array<float, NUM_CASCADES> mSplitDepths{};
		std::array<glm::mat4, NUM_CASCADES> mLightSpaceMatrices{};
		std::array<float, NUM_CASCADES> mCascadeOrthoSizes{};
		std::array<float, NUM_CASCADES> mStableTexelSizes{};
	};
}