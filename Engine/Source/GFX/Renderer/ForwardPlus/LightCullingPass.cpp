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

		glCreateBuffers(1, &mLightIndexListSSBO);
		glNamedBufferStorage(mLightIndexListSSBO,
			sizeof(u32) * tileCount * MAX_LIGHTS_PER_TILE,
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
		mLightIndexListSSBO = 0;
		mLightGridSSBO = 0;
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

		glDispatchCompute(mTileCountX, mTileCountY, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		std::vector<glm::uvec2> readback(mTileCountX * mTileCountY);
		glGetNamedBufferSubData(mLightGridSSBO, 0,
			readback.size() * sizeof(glm::uvec2), readback.data());

		u32 nonZeroTiles = 0;
		u32 maxLights = 0;
		for (const auto& tile : readback)
		{
			u32 total = tile.x + tile.y;
			if (total > 0) nonZeroTiles++;
			maxLights = std::max(maxLights, total);
		}
		//Logger::Internal::info("LightCulling readback: {}/{} tiles lit, max {} lights/tile",
		//	nonZeroTiles, readback.size(), maxLights);
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
		shader->set("uMaxLightsPerTile", MAX_LIGHTS_PER_TILE);

		Logger::Internal::info("Dimensions: {}x{}",
			mWidth, mHeight);

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