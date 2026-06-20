#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Resource/TexturePool.hpp>
#include <WinkEngine/Content/Image.hpp>

namespace Wink::GFX::Resource
{
	TextureHandle TexturePool::load(
		std::string_view path,
		const TextureParams& params)
	{
		Content::ImageData img = Content::load_image(path);
		if (!img) return {};

		return load(img.pixels.data(),
			static_cast<u32>(img.width),
			static_cast<u32>(img.height), params);
	}

	TextureHandle TexturePool::load(
		const u8* pixels, u32 width, u32 height,
		const TextureParams& params)
	{
		TextureHandle handle = allocate();
		with(handle, [&](Texture2D& tex)
			{
				tex.upload(pixels, width, height, params);
			});
		return handle;
	}

	TextureHandle TexturePool::allocate_empty(
		u32 width, u32 height, u32 internalFormat,
		const TextureParams& params)
	{
		TextureHandle handle = allocate();
		with(handle, [&](Texture2D& tex)
			{
				tex.allocate(width, height, internalFormat, params);
			});
		return handle;
	}

	void TexturePool::unload(TextureHandle handle) noexcept
	{
		deallocate(handle);
	}

	void TexturePool::bind(TextureHandle handle, u32 unit) const noexcept
	{
		with(handle, [&](Texture2D& tex) { tex.bind(unit); });
	}

	u32 TexturePool::get_id(TextureHandle handle) const noexcept
	{
		u32 id = 0;
		with(handle, [&](Texture2D& tex) { id = tex.get_id(); });
		return id;
	}

	u32 TexturePool::get_width(TextureHandle handle) const noexcept
	{
		u32 w = 0;
		with(handle, [&](Texture2D& tex) { w = tex.get_width(); });
		return w;
	}

	u32 TexturePool::get_height(TextureHandle handle) const noexcept
	{
		u32 h = 0;
		with(handle, [&](Texture2D& tex) { h = tex.get_height(); });
		return h;
	}

	bool TexturePool::is_valid(TextureHandle handle) const noexcept
	{
		bool valid = ResourcePool::is_valid(handle);
		with(handle, [&](Texture2D& tex) { valid &= tex.is_valid(); });
		return valid;
	}
}