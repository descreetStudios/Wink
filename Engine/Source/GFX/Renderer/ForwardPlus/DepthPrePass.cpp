#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer/ForwardPlus/DepthPrePass.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/RES/MeshPool.hpp>
#include <WinkEngine/GFX/RES/MaterialPool.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>

namespace Wink::GFX
{
	using namespace RES;

	extern MeshPool gMeshPool;
	extern ShaderPool gShaderPool;
	extern MaterialPool gMaterialPool;

	extern u32 gFullscreenVAO;
}

namespace Wink::GFX::ForwardPlus
{
	extern ShaderHandle gDepthOnlyShader;
	extern ShaderHandle gDepthDebugShader;

	bool DepthPrePass::init(u32 width, u32 height) noexcept
	{
		return build_targets(width, height);
	}

	void DepthPrePass::resize(u32 width, u32 height) noexcept
	{
		if (mWidth == width && mHeight == height) return;

		mDepthTex = Texture2D{};
		mFBO = Framebuffer{};

		build_targets(width, height);
	}

	bool DepthPrePass::build_targets(u32 width, u32 height) noexcept
	{
		mWidth = width;
		mHeight = height;

		Texture2DParams depthParams;
		depthParams.wrapS = TextureWrap::ClampToEdge;
		depthParams.wrapT = TextureWrap::ClampToEdge;
		depthParams.minFilter = TextureFilter::Nearest;
		depthParams.magFilter = TextureFilter::Nearest;
		depthParams.genMips = false;
		depthParams.sRGB = false;
		depthParams.hasAlpha = false;

		mDepthTex.allocate(width, height, GL_DEPTH_COMPONENT32F, depthParams);
		mFBO.attach(mDepthTex, Attachment::Depth);

		glNamedFramebufferDrawBuffer(mFBO.get_id(), GL_NONE);
		glNamedFramebufferReadBuffer(mFBO.get_id(), GL_NONE);

		if (!mFBO.check())
		{
			Logger::Internal::error("DepthPrePass framebuffer is incomplete");
			return false;
		}

		return true;
	}

	void DepthPrePass::execute(
		const CameraData& camData,
		std::span<const RenderObject> objects,
		std::span<const glm::mat4> modelMats) noexcept
	{
		assert(objects.size() == modelMats.size());
		ENGINE_ZONE_NAME("DepthPrePass");

		ShaderProgram* shader = gShaderPool.try_get(gDepthOnlyShader);
		if (!shader || !shader->is_valid())
		{
			Logger::Internal::error("No valid depth-only shader");
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, mFBO.get_id());
		glViewport(0, 0, mWidth, mHeight);
		glClear(GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		shader->use();
		shader->set("uViewProj", camData.viewProj);

		for (size_t i = 0; i < objects.size(); ++i)
		{
			const RenderObject& obj = objects[i];

			if (!gMeshPool.is_valid(obj.mesh)) continue;

			shader->set("uModel", modelMats[i]);

			glBindVertexArray(gMeshPool.get_vao_id(obj.mesh));
			glDrawElements(GL_TRIANGLES,
				static_cast<i32>(gMeshPool.get_index_count(obj.mesh)),
				GL_UNSIGNED_INT, nullptr);
		}

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void DepthPrePass::debug_draw(u32 width, u32 height) const noexcept
	{
		ShaderProgram* shader = gShaderPool.try_get(gDepthDebugShader);
		if (!shader || !shader->is_valid()) return;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		glDisable(GL_DEPTH_TEST);

		shader->use();
		glBindTextureUnit(0, mDepthTex.get_id());
		shader->set("uDepth", 0);
		shader->set("uNear", 0.01f);
		shader->set("uFar", 1000.0f);

		glBindVertexArray(gFullscreenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}

	void DepthPrePass::blit_depth_to(u32 dstW, u32 dstH, u32 target) const noexcept
	{
		mFBO.blit_to(target, mWidth, mHeight,
			dstW, dstH, GL_DEPTH_BUFFER_BIT,
			GL_NEAREST);
	}
}