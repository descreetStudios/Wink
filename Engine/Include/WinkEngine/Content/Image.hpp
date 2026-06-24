#pragma once

namespace Wink::Content
{
	struct DecodedImage
	{
		i32 width = 0;
		i32 height = 0;
		i32 channels = 0;
		std::vector<u8> pixels;

		bool is_valid() const noexcept { return !pixels.empty(); }
		explicit operator bool() const noexcept { return is_valid(); }
	};

	struct HDRImage
	{
		i32 width = 0;
		i32 height = 0;
		i32 channels = 0;
		std::vector<float> pixels;

		bool is_valid() const noexcept { return !pixels.empty(); }
		explicit operator bool() const noexcept { return is_valid(); }
	};

	[[nodiscard]] DecodedImage decode_image(
		const fs::path& path,
		bool forceRGBA = true,
		bool flipVertically = true) noexcept;

	[[nodiscard]] DecodedImage decode_image_from_memory(
		const u8* buffer, size_t size,
		bool forceRGBA = true,
		bool flipVertically = true) noexcept;

	[[nodiscard]] HDRImage decode_hdr(
		const fs::path& path,
		bool flipVertically = true) noexcept;

	[[nodiscard]] HDRImage decode_hdr_from_memory(
		const u8* buffer, size_t size,
		bool flipVertically = true) noexcept;
}