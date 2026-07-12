#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline/ShadowPass.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/RES/MeshPool.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>
#include <GLFW/glfw3.h>

namespace Wink::GFX
{
	using namespace RES;

	extern ShaderPool gShaderPool;
	extern MeshPool gMeshPool;
	extern u32 gFullscreenVAO;
}

namespace Wink::GFX::Pipeline
{
	extern ShaderHandle gShadowShader;
	extern ShaderHandle gShadowDebugShader;

	static constexpr u32 SHADOW_MAP_SIZE = 4096;

	ShadowPass::~ShadowPass() noexcept
	{
		glDeleteSamplers(1, &mSamplerCmp);
		glDeleteSamplers(1, &mSamplerRaw);
		glDeleteBuffers(1, &mShadowUBO);
	}

	bool ShadowPass::init() noexcept
	{
		/* --- Shadow map texture array --- */
		Texture2DParams params;
		params.wrapS = TextureWrap::ClampToBorder;
		params.wrapT = TextureWrap::ClampToBorder;
		params.minFilter = TextureFilter::Nearest;
		params.magFilter = TextureFilter::Nearest;
		params.genMips = false;
		params.sRGB = false;
		params.hasAlpha = false;
		mShadowMap.allocate(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
			NUM_CASCADES, GL_DEPTH_COMPONENT32F, params);

		const float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTextureParameterfv(mShadowMap.get_id(), GL_TEXTURE_BORDER_COLOR, border);

		/* --- Samplers --- */
		glCreateSamplers(1, &mSamplerCmp);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glSamplerParameterfv(mSamplerCmp, GL_TEXTURE_BORDER_COLOR, border);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

		glCreateSamplers(1, &mSamplerRaw);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glSamplerParameterfv(mSamplerRaw, GL_TEXTURE_BORDER_COLOR, border);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_COMPARE_MODE, GL_NONE);

		/* --- Shadow UBO --- */
		glCreateBuffers(1, &mShadowUBO);
		glNamedBufferStorage(mShadowUBO, sizeof(ShadowGPUData), nullptr,
			GL_DYNAMIC_STORAGE_BIT);

		/* --- FBO --- */
		glNamedFramebufferDrawBuffer(mFBO.get_id(), GL_NONE);
		glNamedFramebufferReadBuffer(mFBO.get_id(), GL_NONE);

		return true;
	}

	void ShadowPass::compute_splits(float zNear, float zFar, float lambda) noexcept
	{
		for (u32 i = 0; i < NUM_CASCADES; ++i)
		{
			float p = (i + 1.0f) / NUM_CASCADES;
			float lg = zNear * std::pow(zFar / zNear, p);
			float linear = zNear + (zFar - zNear) * p;
			mSplitDepths[i] = lambda * lg + (1.0f - lambda) * linear;
		}
	}

	void ShadowPass::compute_stable_texel_sizes(
		float zNear, float zFar, float lambda,
		float fovY, float aspect) noexcept
	{
		for (u32 i = 0; i < NUM_CASCADES; ++i)
		{
			float splitFar = mSplitDepths[i];
			float halfH = splitFar * std::tan(fovY * 0.5f);
			float halfW = halfH * aspect;
			float maxExtent = 2.0f * std::sqrt(halfW * halfW + halfH * halfH);
			mStableTexelSizes[i] = maxExtent / static_cast<float>(SHADOW_MAP_SIZE);
		}
	}

	void ShadowPass::compute_cascade_matrix(u32 cascade,
		const glm::vec3& lightDir, const glm::mat4& lightView,
		const glm::mat4& invVP, const glm::vec3& cameraPos,
		float zNear, float zFar, float lightOrthoZ,
		float stableTexelSize) noexcept
	{
		const std::array<glm::vec4, 8> ndcCorners{ {
			{-1,-1,-1,1}, { 1,-1,-1,1}, {-1, 1,-1,1}, { 1, 1,-1,1},
			{-1,-1, 1,1}, { 1,-1, 1,1}, {-1, 1, 1,1}, { 1, 1, 1,1}
		} };

		std::array<glm::vec3, 8> frustumCorners{};
		for (u32 i = 0; i < 8; ++i)
		{
			glm::vec4 ws = invVP * ndcCorners[i];
			frustumCorners[i] = glm::vec3(ws) / ws.w;
		}

		float fullRange = zFar - zNear;
		float nearT = ((cascade == 0 ? zNear : mSplitDepths[cascade - 1.0f]) - zNear) / fullRange;
		float farT = (mSplitDepths[cascade] - zNear) / fullRange;

		for (u32 i = 0; i < 4; ++i)
		{
			glm::vec3 ray = frustumCorners[i + 4.0f] - frustumCorners[i];
			glm::vec3 near = frustumCorners[i];
			frustumCorners[i] = near + ray * nearT;
			frustumCorners[i + 4.0f] = near + ray * farT;
		}

		glm::vec3 minLS(FLT_MAX);
		glm::vec3 maxLS(-FLT_MAX);
		for (auto& c : frustumCorners)
		{
			glm::vec3 ls = glm::vec3(lightView * glm::vec4(c, 1.0f));
			minLS = glm::min(minLS, ls);
			maxLS = glm::max(maxLS, ls);
		}

		float maxExtent = stableTexelSize * static_cast<float>(SHADOW_MAP_SIZE);
		float halfExtent = maxExtent * 0.5f;

		glm::vec3 camLS = glm::vec3(lightView * glm::vec4(cameraPos, 1.0f));
		glm::vec2 centerLS = { camLS.x, camLS.y };
		centerLS.x = std::floor(centerLS.x / stableTexelSize) * stableTexelSize;
		centerLS.y = std::floor(centerLS.y / stableTexelSize) * stableTexelSize;

		minLS.x = centerLS.x - halfExtent;
		maxLS.x = centerLS.x + halfExtent;
		minLS.y = centerLS.y - halfExtent;
		maxLS.y = centerLS.y + halfExtent;

		mCascadeOrthoSizes[cascade] = maxExtent;

		const glm::mat4 lightProj = glm::ortho(
			minLS.x, maxLS.x,
			minLS.y, maxLS.y,
			minLS.z - lightOrthoZ * 0.5f,
			maxLS.z + lightOrthoZ * 0.5f);

		mLightSpaceMatrices[cascade] = lightProj * lightView;
	}

	void ShadowPass::execute(
		const DirLight& light,
		std::span<const RenderObject> objects,
		std::span<const glm::mat4> modelMats,
		const glm::mat4& cameraView,
		const glm::mat4& cameraProj,
		const CSMSettings& settings) noexcept
	{
		ENGINE_ZONE_NAME("Shadow Pass");

		ShaderProgram* shader = gShaderPool.try_get(gShadowShader);
		if (!shader || !shader->is_valid())
		{
			Logger::Internal::error("Shadow pass shader is invalid");
			return;
		}

		const glm::vec3 lightDir = glm::normalize(light.direction);
		const glm::vec3 up = (glm::abs(glm::dot(lightDir, glm::vec3(0, 1, 0))) > 0.99f)
			? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
		const glm::mat4 lightView = glm::lookAt(-lightDir, glm::vec3(0.0f), up);
		const glm::mat4 invVP = glm::inverse(cameraProj * cameraView);
		const glm::vec3 cameraPos = glm::vec3(glm::inverse(cameraView)[3]);

		const float fovY = 2.0f * std::atan(1.0f / cameraProj[1][1]);
		const float aspect = cameraProj[1][1] / cameraProj[0][0];

		compute_splits(settings.zNear, settings.zFar, settings.lambda);
		compute_stable_texel_sizes(settings.zNear, settings.zFar, settings.lambda, fovY, aspect);

		for (u32 cascade = 0; cascade < NUM_CASCADES; ++cascade)
		{
			compute_cascade_matrix(cascade, lightDir,
				lightView, invVP, cameraPos,
				settings.zNear, settings.zFar,
				settings.lightOrthoZ, mStableTexelSizes[cascade]);
		}

		mUBOData.shadowMapTexelSize = 1.0f / static_cast<float>(SHADOW_MAP_SIZE);
		for (u32 i = 0; i < NUM_CASCADES; ++i)
		{
			mUBOData.lightSpaceMatrices[i] = mLightSpaceMatrices[i];
			mUBOData.cascadeSplits = glm::vec4(mSplitDepths[0],
				mSplitDepths[1], mSplitDepths[2], mSplitDepths[3]);
			mUBOData.cascadeOrthoSizes = glm::vec4(mCascadeOrthoSizes[0],
				mCascadeOrthoSizes[1], mCascadeOrthoSizes[2], mCascadeOrthoSizes[3]);
		}
		glNamedBufferSubData(mShadowUBO, 0, sizeof(ShadowGPUData), &mUBOData);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDisable(GL_CULL_FACE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(2.0f, 4.0f);

		shader->use();

		for (u32 cascade = 0; cascade < NUM_CASCADES; ++cascade)
		{
			glNamedFramebufferTextureLayer(mFBO.get_id(), GL_DEPTH_ATTACHMENT,
				mShadowMap.get_id(), 0, cascade);
			glBindFramebuffer(GL_FRAMEBUFFER, mFBO.get_id());
			glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
			glClear(GL_DEPTH_BUFFER_BIT);

			shader->set("uLightSpaceMatrix", mLightSpaceMatrices[cascade]);

			for (size_t i = 0; i < objects.size(); ++i)
			{
				if (!gMeshPool.is_valid(objects[i].mesh)) continue;
				shader->set("uModel", modelMats[i]);
				glBindVertexArray(gMeshPool.get_vao_id(objects[i].mesh));
				glDrawElements(GL_TRIANGLES,
					static_cast<i32>(gMeshPool.get_index_count(objects[i].mesh)),
					GL_UNSIGNED_INT, nullptr);
			}
		}

		glDisable(GL_POLYGON_OFFSET_FILL);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void ShadowPass::bind_shadow_map(u32 unit) const noexcept
	{
		glBindTextureUnit(unit, mShadowMap.get_id());
		glBindTextureUnit(unit + 1, mShadowMap.get_id());
		glBindSampler(unit, mSamplerCmp);
		glBindSampler(unit + 1, mSamplerRaw);
		glBindBufferBase(GL_UNIFORM_BUFFER, 4, mShadowUBO);
	}

	void ShadowPass::debug_draw(u32 width, u32 height, u32 cascade) noexcept
	{
		ShaderProgram* shader = gShaderPool.try_get(gShadowDebugShader);
		if (!shader || !shader->is_valid()) return;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		shader->use();
		glBindTextureUnit(0, mShadowMap.get_id());
		glBindSampler(0, mSamplerRaw);
		shader->set("uShadowMap", 0);
		shader->set("uCascadeIndex", static_cast<i32>(cascade));

		glBindVertexArray(gFullscreenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindSampler(0, 0);
	}
}