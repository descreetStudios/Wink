#pragma once

#include <WinkEngine/GFX/Framebuffer.hpp>
#include <WinkEngine/GFX/Texture2D.hpp>
#include <WinkEngine/GFX/Renderer/Data.hpp>

namespace Wink::GFX::Pipeline
{
	class DepthPrePass
	{
	public:
		bool init(u32 width, u32 height) noexcept;
		void resize(u32 width, u32 height) noexcept;
		void execute(const CameraData& camData,
			std::span<const RenderObject> objects,
			std::span<const glm::mat4> modelMats) noexcept;

		void debug_draw(u32 width, u32 height) const noexcept;

		void blit_depth_to(u32 dstW, u32 dstH, u32 target) const noexcept;

		[[nodiscard]] u32 get_depth_id() const noexcept { return mDepthTex.get_id(); }
		[[nodiscard]] u32 get_width() const noexcept { return mWidth; }
		[[nodiscard]] u32 get_height() const noexcept { return mHeight; }
		[[nodiscard]] bool is_valid() const noexcept { return mFBO.is_valid(); }

	private:
		bool build_targets(u32 width, u32 height) noexcept;

	private:
		Framebuffer mFBO;
		Texture2D mDepthTex;

		u32 mWidth = 0;
		u32 mHeight = 0;
	};
}