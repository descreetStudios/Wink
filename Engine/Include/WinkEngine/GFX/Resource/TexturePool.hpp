#pragma once

#include <WinkEngine/GFX/Resource/ResourcePool.hpp>
#include <WinkEngine/GFX/Texture.hpp>
#include <WinkEngine/Core/FileWatcher.hpp>

namespace Wink::GFX::Resource
{
	class TexturePool final : public ResourcePool<Texture2D, TextureTag>
	{
	public:
		TextureHandle load(
			std::string_view path,
			const TextureParams& params = {}, bool hotReload = true);

		TextureHandle load(
			const u8* pixels, u32 width, u32 height,
			const TextureParams& params = {});

		TextureHandle allocate_empty(
			u32 width, u32 height, u32 internalFormat,
			const TextureParams& params = {});

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
			TextureParams params;
		};

		void start_watching(TextureHandle handle) const;
		void stop_watching(TextureHandle handle) const;
		void reload(TextureHandle handle) const;

	private:
		std::unordered_map<u32, ReloadInfo> mReloadSources;
		mutable FileWatcher mWatcher;
	};
}