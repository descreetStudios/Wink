#include <WinkEngine/pch.hpp>
#include <WinkEngine/Content/Image.hpp>
#include <WinkEngine/Core/Logger.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace Wink::Content
{
	DecodedImage decode_image(const fs::path& path,
		bool forceRGBA, bool flipVertically) noexcept
	{
		DecodedImage img;

		if (!fs::exists(path) || !fs::is_regular_file(path))
		{
			Logger::Internal::error(
				"Trying to load an image from an invalid path: '{}'",
				path.string());
			return img;
		}

		stbi_set_flip_vertically_on_load(flipVertically);

		unsigned char* data = stbi_load(
			path.string().c_str(), &img.width, &img.height,
			&img.channels, forceRGBA ? 4 : 0);

		if (!data)
		{
			Logger::Internal::error(
				"Failed to load image '{}': {}",
				path.string(),
				stbi_failure_reason()
			);
			return img;
		}

		if (forceRGBA) img.channels = 4;

		const size_t total =
			static_cast<size_t>(img.width) *
			static_cast<size_t>(img.height) *
			static_cast<size_t>(img.channels);

		img.pixels.assign(data, data + total);
		stbi_image_free(data);
		return img;
	}

	DecodedImage decode_image_from_memory(
		const u8* buffer, size_t size,
		bool forceRGBA, bool flipVertically) noexcept
	{
		DecodedImage img;

		if (!buffer || size == 0)
		{
			Logger::Internal::error(
				"Trying to load image from an invalid buffer or size");
			return img;
		}

		stbi_set_flip_vertically_on_load(flipVertically);

		unsigned char* data = stbi_load_from_memory(
			buffer, static_cast<i32>(size),
			&img.width, &img.height, &img.channels,
			forceRGBA ? 4 : 0
		);

		if (!data)
		{
			Logger::Internal::error(
				"Failed to load image from memory: {}",
				stbi_failure_reason()
			);
			return img;
		}

		if (forceRGBA) img.channels = 4;

		const size_t total =
			static_cast<size_t>(img.width) *
			static_cast<size_t>(img.height) *
			static_cast<size_t>(img.channels);

		img.pixels.assign(data, data + total);
		stbi_image_free(data);
		return img;
	}

	HDRImage decode_hdr(const fs::path& path,
		bool flipVertically) noexcept
	{
		HDRImage img;

		if (!fs::exists(path) || !fs::is_regular_file(path))
		{
			Logger::Internal::error(
				"Trying to load an HDR from an invalid path: '{}'",
				path.string());
			return img;
		}

		stbi_set_flip_vertically_on_load(flipVertically);

		float* data = stbi_loadf(
			path.string().c_str(),
			&img.width, &img.height, &img.channels, 3);

		if (!data)
		{
			Logger::Internal::error(
				"Failed to load HDR image '{}': {}",
				path.string(), stbi_failure_reason());
			return img;
		}

		img.channels = 3;

		const size_t total =
			static_cast<size_t>(img.width) *
			static_cast<size_t>(img.height) *
			static_cast<size_t>(img.channels);

		img.pixels.assign(data, data + total);
		stbi_image_free(data);
		return img;
	}

	HDRImage decode_hdr_from_memory(
		const u8* buffer, size_t size,
		bool flipVertically) noexcept
	{
		HDRImage img;

		if (!buffer || size == 0)
		{
			Logger::Internal::error(
				"Trying to load HDR from an invalid buffer or size");
			return img;
		}

		stbi_set_flip_vertically_on_load(flipVertically);

		float* data = stbi_loadf_from_memory(
			buffer, static_cast<i32>(size),
			&img.width, &img.height, &img.channels, 3);

		if (!data)
		{
			Logger::Internal::error(
				"Failed to load HDR from memory: {}",
				stbi_failure_reason());
			return img;
		}

		img.channels = 3;

		const size_t total =
			static_cast<size_t>(img.width) *
			static_cast<size_t>(img.height) *
			static_cast<size_t>(img.channels);

		img.pixels.assign(data, data + total);
		stbi_image_free(data);
		return img;
	}
}