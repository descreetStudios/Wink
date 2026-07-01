#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/TexturePool.hpp>
#include <WinkEngine/Content/Image.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX::RES
{
	TextureHandle TexturePool::decode(const fs::path& path,
		const Texture2DParams& params, bool hotReload, bool flipVertically)
	{
		if (path.empty())
		{
			Logger::Internal::error("Texture path is invalid");
			return {};
		}

		if (!fs::exists(path))
		{
			Logger::Internal::error(
				"Texture path does not exist '{}'", path.string());
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
		ImageKind kind = ImageKind::LDR;

		if (ext == ".hdr" || ext == ".exr")
		{
			Content::HDRImage hdr = Content::decode_hdr(path);
			if (!hdr)
			{
				Logger::Internal::error(
					"Failed to decode HDR image '{}'", path.string());
				return {};
			}

			handle = load(hdr.pixels.data(),
				static_cast<u32>(hdr.width),
				static_cast<u32>(hdr.height),
				3, TextureDataType::Float, params);

			kind = ImageKind::HDR;
		}
		else if (ext == ".ktx")
		{
			Content::DecodedImage ktx = Content::decode_ktx(path);
			if (!ktx)
			{
				Logger::Internal::error(
					"Failed to decode KTX texture '{}'", path.string());
				return {};
			}

			handle = load(ktx.pixels.data(),
				static_cast<u32>(ktx.width),
				static_cast<u32>(ktx.height),
				static_cast<u32>(ktx.channels),
				TextureDataType::HalfFloat, params);

			kind = ImageKind::KTX;
		}
		else
		{
			Content::DecodedImage img = Content::decode_image(path);
			if (!img)
			{
				Logger::Internal::error(
					"Failed to decode LDR image '{}'", path.string());
				return {};
			}

			handle = load(img.pixels.data(),
				static_cast<u32>(img.width),
				static_cast<u32>(img.height),
				static_cast<u32>(img.channels),
				TextureDataType::UnsignedByte, params);
		}

		if (!handle.is_valid())
			return {};

		mReloadSources[handle] = ReloadInfo{ path, params, kind };
		set_hot_reload(handle, hotReload);
		return handle;
	}

	TextureHandle TexturePool::decode_from_memory(
		const u8* encodedData, size_t size,
		const Texture2DParams& params, bool flipVertically)
	{
		assert(encodedData != nullptr);
		assert(size > 0);

		Content::DecodedImage img = Content::decode_image_from_memory(
			encodedData, size, params.hasAlpha, flipVertically);
		if (!img) return {};

		return load(img.pixels.data(),
			static_cast<u32>(img.width),
			static_cast<u32>(img.height),
			static_cast<u32>(img.channels),
			TextureDataType::UnsignedByte, params);
	}

	TextureHandle TexturePool::decode_hdr_from_memory(
		const u8* encodedData, size_t size,
		const Texture2DParams& params, bool flipVertically)
	{
		assert(encodedData != nullptr);
		assert(size > 0);

		Content::HDRImage hdr = Content::decode_hdr_from_memory(
			encodedData, size, flipVertically);
		if (!hdr) return {};

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
		assert(pixels != nullptr);
		assert(width > 0 && height > 0);

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
		assert(pixels != nullptr);
		assert(width > 0 && height > 0);

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
		assert(width > 0 && height > 0);

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
		mReloadSources.erase(handle);
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
		return get_or(handle, [](Texture2D& tex) { return tex.get_id(); });
	}

	u32 TexturePool::get_width(TextureHandle handle) const noexcept
	{
		return get_or(handle, [](Texture2D& tex) { return tex.get_width(); });
	}

	u32 TexturePool::get_height(TextureHandle handle) const noexcept
	{
		return get_or(handle, [](Texture2D& tex) { return tex.get_height(); });
	}

	bool TexturePool::is_valid(TextureHandle handle) const noexcept
	{
		return ResourcePool::is_valid(handle)
			&& get_or(handle, [](Texture2D& tex) { return tex.is_valid(); });
	}

	void TexturePool::start_watching(TextureHandle handle) const
	{
		auto it = mReloadSources.find(handle);
		if (it == mReloadSources.end()) return;

		mWatcher.watch(it->second.path, [this, handle](const fs::path&)
			{
				reload(handle);
			});
	}

	void TexturePool::stop_watching(TextureHandle handle) const
	{
		auto it = mReloadSources.find(handle);
		if (it == mReloadSources.end()) return;

		mWatcher.unwatch(it->second.path);
	}

	void TexturePool::reload(TextureHandle handle) const
	{
		bool enabled = get_or(handle, [](Texture2D& tex) { return tex.hotReloadEnabled; });
		if (!enabled) return;

		Logger::trace("Hot-reloading texture");

		auto it = mReloadSources.find(handle);
		if (it == mReloadSources.end()) return;

		const ReloadInfo& info = it->second;

		Texture2D fresh;

		switch (info.kind)
		{
		case ImageKind::HDR:
		{
			Content::HDRImage hdr = Content::decode_hdr(info.path);
			if (!hdr)
			{
				Logger::Internal::error(
					"Texture hot-reload failed for handle [{}/{}]; keeping previous texture.",
					handle.index, handle.generation);
				return;
			}

			fresh.upload(hdr.pixels.data(),
				static_cast<u32>(hdr.width),
				static_cast<u32>(hdr.height),
				3, TextureDataType::Float, info.params);
			break;
		}
		case ImageKind::KTX:
		{
			Content::DecodedImage ktx = Content::decode_ktx(info.path);
			if (!ktx)
			{
				Logger::Internal::error(
					"Texture hot-reload failed for handle [{}/{}]; keeping previous texture.",
					handle.index, handle.generation);
				return;
			}

			fresh.upload(ktx.pixels.data(),
				static_cast<u32>(ktx.width),
				static_cast<u32>(ktx.height),
				static_cast<u32>(ktx.channels),
				TextureDataType::HalfFloat, info.params);
			break;
		}
		default:
		{
			Content::DecodedImage img = Content::decode_image(info.path);
			if (!img)
			{
				Logger::Internal::error(
					"Texture hot-reload failed for handle [{}/{}]; keeping previous texture.",
					handle.index, handle.generation);
				return;
			}

			fresh.upload(img.pixels.data(),
				static_cast<u32>(img.width),
				static_cast<u32>(img.height),
				static_cast<u32>(img.channels),
				TextureDataType::UnsignedByte, info.params);
			break;
		}
		}

		fresh.hotReloadEnabled = true;
		const_cast<TexturePool*>(this)->replace(handle, std::move(fresh));
	}
}