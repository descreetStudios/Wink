#pragma once

namespace Wink::Content
{
	struct ImageData
	{
		i32 width = 0;
		i32 height = 0;
		i32 channels = 0;
		std::vector<u8> pixels;

		bool is_valid() const noexcept { return !pixels.empty(); }

		explicit operator bool() const noexcept { return is_valid(); }
	};

	ImageData load_image(
		const fs::path& path,
		bool forceRGBA = true,
		bool flipVertically = true) noexcept;

	ImageData load_image_from_memory(
		const u8* buffer, size_t size,
		bool forceRGBA = true,
		bool flipVertically = true) noexcept;
}