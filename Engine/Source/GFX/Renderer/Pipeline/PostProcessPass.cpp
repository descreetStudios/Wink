#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline/PostProcessPass.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>

namespace Wink::GFX
{
	using namespace RES;

	extern ShaderPool gShaderPool;
	extern u32 gFullscreenVAO;
}

namespace Wink::GFX::Pipeline
{
	extern ShaderHandle gPostProcessShader;

	bool PostProcessPass::init(u32 width, u32 height,
		const PostProcessSettings& settings) noexcept
	{
		mSettings = settings;
		build_targets(width, height);
		return mSceneMSAAFBO.check() && mSceneFBO.check();
	}

	void PostProcessPass::apply_settings(
		const PostProcessSettings& settings) noexcept
	{
		mSettings = settings;
		build_targets(mWidth, mHeight);
	}

	void PostProcessPass::resize(u32 width, u32 height) noexcept
	{
		if (mWidth == width && mHeight == height) return;

		mMSAAColor = Renderbuffer{};
		mMSAADepth = Renderbuffer{};
		mSceneMSAAFBO = Framebuffer{};
		mSceneColor = Texture2D{};
		mSceneDepth = Texture2D{};
		mSceneFBO = Framebuffer{};

		build_targets(width, height);
	}

	void PostProcessPass::build_targets(u32 width, u32 height) noexcept
	{
		mWidth = width;
		mHeight = height;

		const u32 samples = mSettings.msaa.enabled ? mSettings.msaa.sampleCount : 1;

		mMSAAColor.allocate(GL_RGBA16F, width, height, samples);
		mMSAADepth.allocate(GL_DEPTH_COMPONENT32F, width, height, samples);

		mSceneMSAAFBO.attach_renderbuffer(mMSAAColor, Attachment::Color0);
		mSceneMSAAFBO.attach_renderbuffer(mMSAADepth, Attachment::Depth);
		mSceneMSAAFBO.set_draw_buffers({ Attachment::Color0 });

		if (!mSceneMSAAFBO.check())
			Logger::Internal::error("MSAA FBO incomplete");

		Texture2DParams colorParams;
		colorParams.wrapS = TextureWrap::ClampToEdge;
		colorParams.wrapT = TextureWrap::ClampToEdge;
		colorParams.minFilter = TextureFilter::Linear;
		colorParams.magFilter = TextureFilter::Linear;
		colorParams.genMips = false;
		colorParams.sRGB = false;
		colorParams.hasAlpha = true;
		mSceneColor.allocate(width, height, GL_RGBA16F, colorParams);

		Texture2DParams depthParams;
		depthParams.wrapS = TextureWrap::ClampToEdge;
		depthParams.wrapT = TextureWrap::ClampToEdge;
		depthParams.minFilter = TextureFilter::Nearest;
		depthParams.magFilter = TextureFilter::Nearest;
		depthParams.genMips = false;
		depthParams.sRGB = false;
		depthParams.hasAlpha = false;
		mSceneDepth.allocate(width, height, GL_DEPTH_COMPONENT32F, depthParams);

		mSceneFBO.attach(mSceneColor, Attachment::Color0);
		mSceneFBO.attach(mSceneDepth, Attachment::Depth);
		mSceneFBO.set_draw_buffers({ Attachment::Color0 });

		if (!mSceneFBO.check())
			Logger::Internal::error("Scene FBO incomplete");
	}

	void PostProcessPass::bind_scene_fbo() const noexcept
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mSceneMSAAFBO.get_id());
	}

	void PostProcessPass::execute(u32 width, u32 height) const noexcept
	{
		ENGINE_ZONE_NAME("Post-Process Pass");

		mSceneMSAAFBO.blit_to(mSceneFBO.get_id(),
			mWidth, mHeight, mWidth, mHeight,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);

		mSceneMSAAFBO.blit_to(mSceneFBO.get_id(),
			mWidth, mHeight, mWidth, mHeight,
			GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		ShaderProgram* shader = gShaderPool.try_get(gPostProcessShader);
		if (!shader || !shader->is_valid())
		{
			Logger::Internal::error("Post-Process shader is invalid");
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		shader->use();

		glBindTextureUnit(0, mSceneColor.get_id());
		shader->set("uSceneColor", 0);
		glBindTextureUnit(1, mSceneDepth.get_id());
		shader->set("uSceneDepth", 1);

		// TODO: Bind here for post-process effects

		glBindVertexArray(gFullscreenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}
}