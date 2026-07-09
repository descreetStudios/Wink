#pragma once

#include <WinkEngine/GFX/Framebuffer.hpp>
#include <WinkEngine/GFX/Renderbuffer.hpp>
#include <WinkEngine/GFX/Texture2D.hpp>

namespace Wink::GFX::Pipeline
{
	struct MSAASettings
	{
		bool enabled = true;
		u32 sampleCount = 4;
	};

	struct PostProcessSettings
	{
		MSAASettings msaa;
	};

	class PostProcessPass
	{
	public:
		bool init(u32 width, u32 height,
			const PostProcessSettings& settings = {}) noexcept;
		void resize(u32 width, u32 height) noexcept;

		void bind_scene_fbo() const noexcept;
		void execute(u32 width, u32 height) const noexcept;
		void apply_settings(const PostProcessSettings& settings) noexcept;

		[[nodiscard]] u32 get_scene_fbo_id() const noexcept { return mSceneMSAAFBO.get_id(); }
		[[nodiscard]] u32 get_scene_depth_id() const noexcept { return mSceneDepth.get_id(); }
		[[nodiscard]] const PostProcessSettings& get_settings() const noexcept { return mSettings; }

	private:
		void build_targets(u32 width, u32 height) noexcept;

	private:
		PostProcessSettings mSettings;

		Framebuffer mSceneMSAAFBO;
		Renderbuffer mMSAAColor;
		Renderbuffer mMSAADepth;

		Framebuffer mSceneFBO;
		Texture2D mSceneColor;
		Texture2D mSceneDepth;

		u32 mWidth = 0;
		u32 mHeight = 0;
	};
}