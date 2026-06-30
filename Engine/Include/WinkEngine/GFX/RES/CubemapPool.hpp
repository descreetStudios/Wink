#pragma once

#include <WinkEngine/GFX/RES/ResourcePool.hpp>
#include <WinkEngine/GFX/TextureCubemap.hpp>
#include <WinkEngine/GFX/Texture2D.hpp>

namespace Wink::GFX::RES
{
	class CubemapPool final : public ResourcePool<TextureCubemap, CubemapTag>
	{
	public:
		CubemapHandle hdr_to_cubemap(TextureHandle hdr, u32 faceSize = 512);
		void unload(CubemapHandle handle) noexcept;

		/* Cubemap operations */
		void bind(CubemapHandle handle, u32 unit = 0) const noexcept;

		[[nodiscard]] u32 get_id(CubemapHandle handle) const noexcept;
		[[nodiscard]] u32 get_width(CubemapHandle handle) const noexcept;
		[[nodiscard]] u32 get_height(CubemapHandle handle) const noexcept;
		[[nodiscard]] u32 get_internal_format(CubemapHandle handle) const noexcept;
		[[nodiscard]] u32 get_mip_levels(CubemapHandle handle) const noexcept;
		[[nodiscard]] bool is_valid(CubemapHandle handle) const noexcept;
	};
}