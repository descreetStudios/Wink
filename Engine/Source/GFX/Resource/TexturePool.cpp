#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Resource/TexturePool.hpp>
#include <WinkEngine/Content/Image.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX::Resource
{
	TextureHandle TexturePool::decode(const fs::path& path,
		const Texture2DParams& params, bool hotReload)
	{
		if (path.empty())
		{
			Logger::Internal::error("Texture path is invalid");
			return {};
		}

		if (!fs::exists(path))
		{
			Logger::Internal::error(
				"Tetxure path does not exist '{}'", path.string());
			return {};
		}

		if (!fs::is_regular_file(path))
		{
			Logger::Internal::error(
				"Texture path is not a regular file '{}'", path.string());
			return {};
		}

		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		TextureHandle handle;

		if (ext == ".hdr" || ext == ".exr")
		{
			Content::HDRImage hdr = Content::decode_hdr(path);
			if (!hdr)
			{
				Logger::Internal::error(
					"Failed to decode HDR image '{}'", path.string());
				return handle;
			}

			handle = load(hdr.pixels.data(),
				static_cast<u32>(hdr.width),
				static_cast<u32>(hdr.height),
				3, TextureDataType::Float, params);
			if (!handle.is_valid())
				return handle;

			mReloadSources[handle.index] = ReloadInfo{ path, params };
			set_hot_reload(handle, hotReload);
			return handle;
		}

		if (ext == ".ktx")
		{
			Content::DecodedImage ktx = Content::decode_ktx(path);
			if (!ktx)
			{
				Logger::Internal::error(
					"Failed to decode KTX texture '{}'", path.string());
				return handle;
			}

			handle = load(ktx.pixels.data(),
				static_cast<u32>(ktx.width),
				static_cast<u32>(ktx.height),
				static_cast<u32>(ktx.channels),
				TextureDataType::HalfFloat, params);

			if (!handle.is_valid())
				return handle;

			mReloadSources[handle.index] = ReloadInfo{ path, params };
			set_hot_reload(handle, hotReload);
			return handle;
		}

		Content::DecodedImage img = Content::decode_image(path);
		if (!img)
		{
			Logger::Internal::error(
				"Failed to decode LDR image '{}'", path.string());
			return handle;
		}

		handle = load(img.pixels.data(),
			static_cast<u32>(img.width),
			static_cast<u32>(img.height),
			static_cast<u32>(img.channels),
			TextureDataType::UnsignedInt, params);
		if (!handle.is_valid())
			return handle;

		mReloadSources[handle.index] = ReloadInfo{ path, params };
		set_hot_reload(handle, hotReload);
		return handle;
	}

	TextureHandle TexturePool::decode_from_memory(
		const u8* encodedData, size_t size,
		const Texture2DParams& params)
	{
		Content::DecodedImage img = Content::decode_image_from_memory(
			encodedData, size, params.hasAlpha);
		if (!img) return TextureHandle();

		return load(img.pixels.data(),
			static_cast<u32>(img.width),
			static_cast<u32>(img.height),
			static_cast<u32>(img.channels),
			TextureDataType::UnsignedByte, params);
	}

	TextureHandle TexturePool::decode_hdr_from_memory(
		const u8* encodedData, size_t size,
		const Texture2DParams& params)
	{
		Content::HDRImage hdr = Content::decode_hdr_from_memory(
			encodedData, size, params.hasAlpha);
		if (!hdr) return TextureHandle();

		return load(hdr.pixels.data(),
			static_cast<u32>(hdr.width),
			static_cast<u32>(hdr.height),
			static_cast<u32>(hdr.channels),
			TextureDataType::Float, params);
	}

	TextureHandle TexturePool::load(
		const u8* pixels, u32 width, u32 height,
		u32 channels, TextureDataType dataType,
		const Texture2DParams& params)
	{
		TextureHandle handle = allocate();
		with(handle, [&](Texture2D& tex)
			{
				tex.upload(pixels, width, height, channels, dataType, params);
			});
		return handle;
	}

	TextureHandle TexturePool::load(
		const float* pixels, u32 width, u32 height,
		u32 channels, TextureDataType dataType,
		const Texture2DParams& params)
	{
		TextureHandle handle = allocate();
		with(handle, [&](Texture2D& tex)
			{
				tex.upload(pixels, width, height, channels, dataType, params);
			});
		return handle;
	}

	TextureHandle TexturePool::allocate_empty(
		u32 width, u32 height, u32 internalFormat,
		const Texture2DParams& params)
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
		stop_watching(handle);
		mReloadSources.erase(handle.index);
		deallocate(handle);
	}

	void TexturePool::set_hot_reload(
		TextureHandle handle, bool enabled) const noexcept
	{
		with(handle, [&](Texture2D& tex) { tex.hotReloadEnabled = enabled; });

		if (enabled) start_watching(handle);
		else stop_watching(handle);
	}

	void TexturePool::poll_hot_reload()
	{
		mWatcher.poll();
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

	void TexturePool::start_watching(TextureHandle handle) const
	{
		auto it = mReloadSources.find(handle.index);
		if (it == mReloadSources.end()) return;

		mWatcher.watch(it->second.path, [this, handle](const fs::path&)
			{
				reload(handle);
			});
	}

	void TexturePool::stop_watching(TextureHandle handle) const
	{
		auto it = mReloadSources.find(handle.index);
		if (it == mReloadSources.end()) return;

		mWatcher.unwatch(it->second.path);
	}

	void TexturePool::reload(TextureHandle handle) const
	{
		bool enabled = false;
		with(handle, [&](Texture2D& tex) { enabled = tex.hotReloadEnabled; });
		if (!enabled) return;

		Logger::trace("Hot-reloading texture");

		auto it = mReloadSources.find(handle.index);
		if (it == mReloadSources.end()) return;

		auto img = Content::decode_image(it->second.path);
		if (!img.is_valid())
		{
			Logger::Internal::error(
				"Texture hot-reload failed for handle index '{}'; keeping previous texture.",
				handle.index);
			return;
		}

		Texture2D fresh;
		fresh.upload(img.pixels.data(),
			static_cast<u32>(img.width),
			static_cast<u32>(img.height),
			static_cast<u32>(img.channels),
			TextureDataType::UnsignedInt,
			it->second.params);
		fresh.hotReloadEnabled = true;

		const_cast<TexturePool*>(this)->replace(handle, std::move(fresh));
	}
}