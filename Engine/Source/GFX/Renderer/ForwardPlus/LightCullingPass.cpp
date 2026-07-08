#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer/ForwardPlus/LightCullingPass.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/GFX/Renderer/GPUData.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>

namespace Wink::GFX
{
	using namespace RES;

	extern ShaderPool gShaderPool;
	extern u32 gFullscreenVAO;
}

namespace Wink::GFX::ForwardPlus
{
	extern ShaderHandle gLightCullingShader;
	extern ShaderHandle gLightHeatmapShader;

	constexpr u32 MAX_POINT_LIGHTS = 64000;
	constexpr u32 MAX_SPOT_LIGHTS = 64000;

	void LightCullingPass::init(u32 width, u32 height) noexcept
	{
		glCreateBuffers(1, &mPointLightSSBO);
		glNamedBufferStorage(mPointLightSSBO,
			sizeof(PointLightGPU) * MAX_POINT_LIGHTS,
			nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &mSpotLightSSBO);
		glNamedBufferStorage(mSpotLightSSBO,
			sizeof(SpotLightGPU) * MAX_SPOT_LIGHTS,
			nullptr, GL_DYNAMIC_STORAGE_BIT);

		build_buffers(width, height);
	}

	void LightCullingPass::resize(u32 width, u32 height) noexcept
	{
		if (mWidth == width && mHeight == height) return;
		destroy_buffers();
		build_buffers(width, height);
	}

	void LightCullingPass::build_buffers(u32 width, u32 height) noexcept
	{
		mWidth = width;
		mHeight = height;
		mTileCountX = (width + TILE_SIZE - 1) / TILE_SIZE;
		mTileCountY = (height + TILE_SIZE - 1) / TILE_SIZE;

		const u32 tileCount = mTileCountX * mTileCountY;

		glCreateBuffers(1, &mGlobalLightCountSSBO);
		glNamedBufferStorage(mGlobalLightCountSSBO,
			sizeof(u32), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &mLightIndexListSSBO);
		glNamedBufferStorage(mLightIndexListSSBO,
			sizeof(u32) * tileCount * 1024,
			nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &mLightGridSSBO);
		glNamedBufferStorage(mLightGridSSBO,
			sizeof(glm::uvec2) * tileCount,
			nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	void LightCullingPass::destroy_buffers() noexcept
	{
		glDeleteBuffers(1, &mLightIndexListSSBO);
		glDeleteBuffers(1, &mLightGridSSBO);
		glDeleteBuffers(1, &mGlobalLightCountSSBO);
		mLightIndexListSSBO = 0;
		mLightGridSSBO = 0;
		mGlobalLightCountSSBO = 0;
	}

	void LightCullingPass::execute(
		const CameraData& camData,
		std::span<const PointLight> pointLights,
		std::span<const SpotLight> spotLights,
		u32 depthTexID) const noexcept
	{
		ENGINE_ZONE_NAME("Light Culling Pass");

		ShaderProgram* shader = gShaderPool.try_get(gLightCullingShader);
		if (!shader || !shader->is_valid())
		{
			Logger::Internal::error("Compute shader for light culling pass invalid");
			return;
		}

		{ /* --- Upload Point Lights --- */
			std::vector<PointLightGPU> gpuPoints;
			gpuPoints.reserve(pointLights.size());
			for (const auto& l : pointLights)
				gpuPoints.push_back({
					glm::vec4(l.position, l.intensity),
					glm::vec4(l.color, l.radius),
					});
			glNamedBufferSubData(mPointLightSSBO, 0,
				sizeof(PointLightGPU) * gpuPoints.size(),
				gpuPoints.data());
		}

		{ /* --- Upload Spot Lights --- */
			std::vector<SpotLightGPU> gpuSpots;
			gpuSpots.reserve(spotLights.size());
			for (const auto& l : spotLights)
				gpuSpots.push_back({
					glm::vec4(l.position, l.range),
					glm::vec4(l.direction, l.intensity),
					glm::vec4(l.color, l.outerCutoff),
					glm::vec4(l.innerCutoff, 0.0f, 0.0f, 0.0f),
					});
			glNamedBufferSubData(mSpotLightSSBO, 0,
				sizeof(SpotLightGPU) * gpuSpots.size(),
				gpuSpots.data());
		}

		/* --- Clear Light Grid Counts --- */
		glClearNamedBufferData(mGlobalLightCountSSBO,
		    GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
		glClearNamedBufferData(mLightGridSSBO,
		    GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);

		/* --- Bind Resources --- */
		shader->use();

		shader->set("uView", camData.view);
		shader->set("uInvProj", camData.invProj);
		shader->set("uScreenWidth", mWidth);
		shader->set("uScreenHeight", mHeight);
		shader->set("uTileCountX", mTileCountX);
		shader->set("uPointLightCount", static_cast<u32>(pointLights.size()));
		shader->set("uSpotLightCount", static_cast<u32>(spotLights.size()));

		glBindTextureUnit(0, depthTexID);
		shader->set("uDepth", 0);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mPointLightSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mSpotLightSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mLightIndexListSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mLightGridSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mGlobalLightCountSSBO);

		glDispatchCompute(mTileCountX, mTileCountY, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	void LightCullingPass::debug_draw(u32 width, u32 height) const noexcept
	{
		ShaderProgram* shader = gShaderPool.try_get(gLightHeatmapShader);
		if (!shader || !shader->is_valid()) return;

		shader->use();
		shader->set("uTileCountX", mTileCountX);
		shader->set("uTileCountY", mTileCountY);
		shader->set("uScreenWidth", mWidth);
		shader->set("uScreenHeight", mHeight);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mLightGridSSBO);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindVertexArray(gFullscreenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
}