#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/CubemapPool.hpp>
#include <WinkEngine/GFX/Renderer/IBL.hpp>

namespace Wink::GFX::RES
{
	CubemapHandle CubemapPool::hdr_to_cubemap(TextureHandle hdr, u32 faceSize)
	{
		return IBL::equirect_to_cubemap(hdr, faceSize);
	}

	void CubemapPool::unload(CubemapHandle handle) noexcept
	{
		deallocate(handle);
	}

	void CubemapPool::bind(CubemapHandle handle, u32 unit) const noexcept
	{
		with(handle, [&](TextureCubemap& tex) { tex.bind(unit); });
	}

	u32 CubemapPool::get_id(CubemapHandle handle) const noexcept
	{
		return get_or(handle, [](TextureCubemap& tex) { return tex.get_id(); });
	}

	u32 CubemapPool::get_width(CubemapHandle handle) const noexcept
	{
		return get_or(handle, [](TextureCubemap& tex) { return tex.get_width(); });
	}

	u32 CubemapPool::get_height(CubemapHandle handle) const noexcept
	{
		return get_or(handle, [](TextureCubemap& tex) { return tex.get_height(); });
	}

	u32 CubemapPool::get_internal_format(CubemapHandle handle) const noexcept
	{
		return get_or(handle, [](TextureCubemap& tex) { return tex.get_internal_format(); });
	}

	u32 CubemapPool::get_mip_levels(CubemapHandle handle) const noexcept
	{
		return get_or(handle, [](TextureCubemap& tex) { return tex.get_mip_levels(); });
	}

	bool CubemapPool::is_valid(CubemapHandle handle) const noexcept
	{
		return ResourcePool::is_valid(handle)
			&& get_or(handle, [](TextureCubemap& tex) { return tex.is_valid(); });
	}
}