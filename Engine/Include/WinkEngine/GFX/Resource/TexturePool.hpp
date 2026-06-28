#pragma once

#include <WinkEngine/GFX/Resource/ResourcePool.hpp>
#include <WinkEngine/GFX/Texture2D.hpp>
#include <WinkEngine/Core/FileWatcher.hpp>

namespace Wink::GFX::Resource
{
	class TexturePool final : public ResourcePool<Texture2D, TextureTag>
	{
	public:
		TextureHandle decode(const fs::path& path,
			const Texture2DParams& params = {},
			bool hotReload = true);

		TextureHandle decode_from_memory(
			const u8* encodedData, size_t size,
			const Texture2DParams& params = {});

		TextureHandle decode_hdr_from_memory(
			const u8* encodedData, size_t size,
			const Texture2DParams& params = {});

		TextureHandle load(const u8* pixels, u32 width,
			u32 height, u32 channels = 3,
			TextureDataType dataType = TextureDataType::UnsignedInt,
			const Texture2DParams& params = {});

		TextureHandle load(const float* pixels, u32 width,
			u32 height, u32 channels = 3,
			TextureDataType dataType = TextureDataType::Float,
			const Texture2DParams& params = {});

		TextureHandle allocate_empty(
			u32 width, u32 height, u32 internalFormat,
			const Texture2DParams& params = {});

		void unload(TextureHandle handle) noexcept;

		void set_hot_reload(TextureHandle handle, bool enabled) const noexcept;
		void poll_hot_reload();

		/* Texture operations */
		void bind(TextureHandle handle, u32 unit = 0) const noexcept;

		[[nodiscard]] u32 get_id(TextureHandle handle) const noexcept;
		[[nodiscard]] u32 get_width(TextureHandle handle) const noexcept;
		[[nodiscard]] u32 get_height(TextureHandle handle) const noexcept;
		[[nodiscard]] bool is_valid(TextureHandle handle) const noexcept;

	private:
		struct ReloadInfo
		{
			fs::path path;
			Texture2DParams params;
		};

		void start_watching(TextureHandle handle) const;
		void stop_watching(TextureHandle handle) const;
		void reload(TextureHandle handle) const;

	private:
		std::unordered_map<u32, ReloadInfo> mReloadSources;
		mutable FileWatcher mWatcher;
	};
}