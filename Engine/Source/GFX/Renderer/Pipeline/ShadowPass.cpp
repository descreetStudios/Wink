#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline/ShadowPass.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/RES/MeshPool.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>

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
			GL_DEPTH_COMPONENT32F, params);

		const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTextureParameterfv(mShadowMap.get_id(),
			GL_TEXTURE_BORDER_COLOR, borderColor);

		glTextureParameteri(mShadowMap.get_id(),
			GL_TEXTURE_COMPARE_FUNC, GL_LESS);

		mFBO.attach(mShadowMap, Attachment::Depth);
		glNamedFramebufferDrawBuffer(mFBO.get_id(), GL_NONE);
		glNamedFramebufferReadBuffer(mFBO.get_id(), GL_NONE);

		if (!mFBO.check())
		{
			Logger::Internal::error("Shadow pass FBO incomplete");
			return false;
		}

		return true;
	}

	void ShadowPass::execute(
		const DirLight& light,
		std::span<const RenderObject> objects,
		std::span<const glm::mat4> modelMats,
		const ShadowOrthoSettings& ortho) noexcept
	{
		ENGINE_ZONE_NAME("Shadow Pass");

		ShaderProgram* shader = gShaderPool.try_get(gShadowShader);
		if (!shader || !shader->is_valid())
		{
			Logger::Internal::error("Shadow pass shader is invalid");
			return;
		}

		const glm::vec3 lightDir = glm::normalize(light.direction);
		const glm::vec3 lightPos = -lightDir * 100.0f;

		const glm::vec3 up = (glm::abs(glm::dot(
			lightDir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f)
			? glm::vec3(1.0f, 0.0f, 0.0f)
			: glm::vec3(0.0f, 1.0f, 0.0f);

		const glm::mat4 lightView = glm::lookAt(
			lightPos, lightPos + lightDir, up);
		const glm::mat4 lightProj = glm::ortho(
			ortho.left, ortho.right,
			ortho.bottom, ortho.top,
			ortho.zNear, ortho.zFar);

		mLightSpaceMatrix = lightProj * lightView;

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
		shader->set("uLightSpaceMatrix", mLightSpaceMatrix);

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
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void ShadowPass::bind_shadow_map(ShaderHandle shader, u32 unit) const noexcept
	{
		glTextureParameteri(mShadowMap.get_id(),
			GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

		glBindTextureUnit(unit, mShadowMap.get_id());

		ShaderProgram* sh = gShaderPool.try_get(shader);
		if (!sh || !sh->is_valid()) return;

		sh->use();
		sh->set("uShadowMap", static_cast<i32>(unit));
		sh->set("uLightSpaceMatrix", mLightSpaceMatrix);
	}

	void ShadowPass::debug_draw(u32 width, u32 height) noexcept
	{
		ShaderProgram* shader = gShaderPool.try_get(gShadowDebugShader);
		if (!shader || !shader->is_valid()) return;

		glTextureParameteri(mShadowMap.get_id(),
			GL_TEXTURE_COMPARE_MODE, GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		shader->use();
		glBindTextureUnit(0, mShadowMap.get_id());
		shader->set("uShadowMap", 0);

		glBindVertexArray(gFullscreenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glTextureParameteri(mShadowMap.get_id(),
			GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}
}