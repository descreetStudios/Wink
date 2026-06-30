#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/CubemapPool.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink::GFX::RES
{
	CubemapHandle CubemapPool::hdr_to_cubemap(TextureHandle hdr, u32 faceSize)
	{
		return IBL::Internal::equirect_to_cubemap(hdr, faceSize);
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
		u32 id = 0;
		with(handle, [&](TextureCubemap& tex) { id = tex.get_id(); });
		return id;
	}

	u32 CubemapPool::get_width(CubemapHandle handle) const noexcept
	{
		u32 w = 0;
		with(handle, [&](TextureCubemap& tex) { w = tex.get_width(); });
		return w;
	}

	u32 CubemapPool::get_height(CubemapHandle handle) const noexcept
	{
		u32 h = 0;
		with(handle, [&](TextureCubemap& tex) { h = tex.get_height(); });
		return h;
	}

	u32 CubemapPool::get_internal_format(CubemapHandle handle) const noexcept
	{
		u32 f = 0;
		with(handle, [&](TextureCubemap& tex) { f = tex.get_internal_format(); });
		return f;
	}

	u32 CubemapPool::get_mip_levels(CubemapHandle handle) const noexcept
	{
		u32 l = 0;
		with(handle, [&](TextureCubemap& tex) { l = tex.get_mip_levels(); });
		return l;
	}

	bool CubemapPool::is_valid(CubemapHandle handle) const noexcept
	{
		bool valid = ResourcePool::is_valid(handle);
		with(handle, [&](TextureCubemap& tex) { valid &= tex.is_valid(); });
		return valid;
	}
}