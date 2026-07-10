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

	ShadowPass::~ShadowPass() noexcept
	{
		glDeleteSamplers(1, &mSamplerCmp);
		glDeleteSamplers(1, &mSamplerRaw);
	}

	bool ShadowPass::init() noexcept
	{
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

		const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTextureParameterfv(mShadowMap.get_id(),
			GL_TEXTURE_BORDER_COLOR, borderColor);

		// TODO: Try to remove these 2 lines
		glTextureParameteri(mShadowMap.get_id(),
			GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glTextureParameteri(mShadowMap.get_id(),
			GL_TEXTURE_COMPARE_FUNC, GL_LESS);

		glCreateSamplers(1, &mSamplerCmp);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glSamplerParameterfv(mSamplerCmp, GL_TEXTURE_BORDER_COLOR, borderColor);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glSamplerParameteri(mSamplerCmp, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

		glCreateSamplers(1, &mSamplerRaw);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glSamplerParameterfv(mSamplerRaw, GL_TEXTURE_BORDER_COLOR, borderColor);
		glSamplerParameteri(mSamplerRaw, GL_TEXTURE_COMPARE_MODE, GL_NONE);

		glNamedFramebufferDrawBuffer(mFBO.get_id(), GL_NONE);
		glNamedFramebufferReadBuffer(mFBO.get_id(), GL_NONE);

		return true;
	}

	void ShadowPass::compute_splits(float zNear, float zFar, float lambda) noexcept
	{
		for (u32 i = 0; i < NUM_CASCADES; ++i)
		{
			float p = static_cast<float>(i + 1) / static_cast<float>(NUM_CASCADES);
			float log = zNear * std::pow(zFar / zNear, p);
			float linear = zNear + (zFar - zNear) * p;
			mSplitDepths[i] = lambda * log + (1.f - lambda) * linear;
		}
	}

	void ShadowPass::compute_cascade_matrix(
		u32 cascade,
		const glm::vec3& lightDir,
		const glm::mat4& cameraView,
		const glm::mat4& cameraProj,
		float zNear, float zFar,
		float lightOrthoZ) noexcept
	{
		const glm::mat4 invVP = glm::inverse(cameraProj * cameraView);

		const std::array<glm::vec3, 8> ndcCorners{ {
			{-1, -1, -1}, {1, -1, -1}, {-1, 1,-1}, {1, 1, -1},
			{-1, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 1}
		} };

		std::array<glm::vec3, 8> frustumCorners{};
		for (u32 i = 0; i < 8; ++i)
		{
			glm::vec4 ws = invVP * glm::vec4(ndcCorners[i], 1.f);
			frustumCorners[i] = glm::vec3(ws) / ws.w;
		}

		float prevSplit = (cascade == 0) ? zNear : mSplitDepths[cascade - 1];
		float currSplit = mSplitDepths[cascade];

		float fullRange = zFar - zNear;
		float nearT = (prevSplit - zNear) / fullRange;
		float farT = (currSplit - zNear) / fullRange;

		for (u32 i = 0; i < 4; ++i)
		{
			glm::vec3 ray = frustumCorners[i + 4] - frustumCorners[i];
			frustumCorners[i] = frustumCorners[i] + ray * nearT;
			frustumCorners[i + 4] = frustumCorners[i] + ray * (farT - nearT);
		}

		// Centroid of the sub-frustum
		glm::vec3 center(0.f);
		for (auto& c : frustumCorners) center += c;
		center /= 8.f;

		// Build light view matrix
		const glm::vec3 up = (glm::abs(glm::dot(lightDir, glm::vec3(0, 1, 0))) > 0.99f)
			? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
		const glm::mat4 lightView = glm::lookAt(center - lightDir, center, up);

		// AABB in light space
		glm::vec3 minLS(FLT_MAX);
		glm::vec3 maxLS(-FLT_MAX);
		for (auto& c : frustumCorners)
		{
			glm::vec3 ls = glm::vec3(lightView * glm::vec4(c, 1.f));
			minLS = glm::min(minLS, ls);
			maxLS = glm::max(maxLS, ls);
		}

		// Store ortho XY size for PCSS light size scaling
		mCascadeOrthoSizes[cascade] = maxLS.x - minLS.x;

		// Snap to texel grid
		float worldUnitsPerTexel = mCascadeOrthoSizes[cascade] / static_cast<float>(SHADOW_MAP_SIZE);
		minLS.x = std::floor(minLS.x / worldUnitsPerTexel) * worldUnitsPerTexel;
		minLS.y = std::floor(minLS.y / worldUnitsPerTexel) * worldUnitsPerTexel;
		maxLS.x = std::floor(maxLS.x / worldUnitsPerTexel) * worldUnitsPerTexel;
		maxLS.y = std::floor(maxLS.y / worldUnitsPerTexel) * worldUnitsPerTexel;

		minLS.z -= lightOrthoZ * 0.5f;
		maxLS.z += lightOrthoZ * 0.5f;

		const glm::mat4 lightProj = glm::ortho(
			minLS.x, maxLS.x,
			minLS.y, maxLS.y,
			-maxLS.z, -minLS.z);

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

		compute_splits(settings.zNear, settings.zFar, settings.lambda);

		for (u32 cascade = 0; cascade < NUM_CASCADES; ++cascade)
		{
			compute_cascade_matrix(cascade, lightDir,
				cameraView, cameraProj,
				settings.zNear, settings.zFar,
				settings.lightOrthoZ);

			// Attach this cascade's layer to the FBO depth attachment
			glNamedFramebufferTextureLayer(
				mFBO.get_id(), GL_DEPTH_ATTACHMENT,
				mShadowMap.get_id(), 0, cascade);

			glBindFramebuffer(GL_FRAMEBUFFER, mFBO.get_id());
			glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
			glClear(GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

			glDisable(GL_CULL_FACE);
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(2.0f, 4.0f);

			shader->use();
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

			glDisable(GL_POLYGON_OFFSET_FILL);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void ShadowPass::bind_shadow_map(ShaderHandle shader, u32 unit) const noexcept
	{
		glBindTextureUnit(unit, mShadowMap.get_id());
		glBindTextureUnit(unit + 1, mShadowMap.get_id());
		glBindSampler(unit, mSamplerCmp);
		glBindSampler(unit + 1, mSamplerRaw);

		ShaderProgram* sh = gShaderPool.try_get(shader);
		if (!sh || !sh->is_valid()) return;

		sh->use();
		sh->set("uShadowMap", static_cast<i32>(unit));
		sh->set("uShadowMapRaw", static_cast<i32>(unit + 1));

		for (u32 i = 0; i < NUM_CASCADES; ++i)
		{
			sh->set("uLightSpaceMatrices[" + std::to_string(i) + "]",
				mLightSpaceMatrices[i]);
			sh->set("uCascadeSplits[" + std::to_string(i) + "]",
				mSplitDepths[i]);
			sh->set("uCascadeOrthoSizes[" + std::to_string(i) + "]",
				mCascadeOrthoSizes[i]);
		}
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