#pragma once

#include <WinkEngine/GFX/Renderer/Data.hpp>

namespace Wink::GFX::ForwardPlus
{
	inline constexpr u32 TILE_SIZE = 16;

	class LightCullingPass
	{
	public:
		void init(u32 width, u32 height) noexcept;
		void resize(u32 width, u32 height) noexcept;
		void execute(
			const CameraData& camData,
			std::span<const PointLight> pointLights,
			std::span<const SpotLight> spotLights,
			u32 depthTexID) const noexcept;

		void debug_draw(u32 width, u32 height) const noexcept;

		[[nodiscard]] u32 get_light_index_list_ssbo() const noexcept { return mLightIndexListSSBO; }
		[[nodiscard]] u32 get_light_grid_ssbo() const noexcept { return mLightGridSSBO; }
		[[nodiscard]] u32 get_tile_count_x() const noexcept { return mTileCountX; }
		[[nodiscard]] u32 get_tile_count_y() const noexcept { return mTileCountY; }

	private:
		void build_buffers(u32 width, u32 height) noexcept;
		void destroy_buffers() noexcept;

	private:
		u32 mLightIndexListSSBO = 0;
		u32 mLightGridSSBO = 0;
		u32 mGlobalLightCountSSBO = 0;

		u32 mPointLightSSBO = 0;
		u32 mSpotLightSSBO = 0;

		u32 mTileCountX = 0;
		u32 mTileCountY = 0;
		u32 mWidth = 0;
		u32 mHeight = 0;
	};
}