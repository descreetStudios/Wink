#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Texture.hpp>

namespace Wink::GFX
{
	Texture2D::Texture2D()
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &mID);
	}

	Texture2D::~Texture2D()
	{
		glDeleteTextures(1, &mID);
	}

	MOVE_CTOR_IMPL(Texture2D) noexcept
		: mID(o.mID), mWidth(o.mWidth), mHeight(o.mHeight)
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(Texture2D) noexcept
	{
		if (this != &o)
		{
			glDeleteTextures(1, &mID);
			mID = o.mID;
			mWidth = o.mWidth;
			mHeight = o.mHeight;
			o.mID = 0;
		}
		return *this;
	}

	void Texture2D::upload(const u8* pixels,
		u32 width, u32 height,
		const TextureParams& params) noexcept
	{
		mWidth = width;
		mHeight = height;

		u32 internal =
			params.sRGB
			? (params.hasAlpha ? GL_SRGB8_ALPHA8 : GL_SRGB8)
			: (params.hasAlpha ? GL_RGBA8 : GL_RGB8);

		glTextureStorage2D(mID, params.genMips ? 8 : 1,
			internal, width, height);

		glTextureSubImage2D(
			mID, 0, 0, 0, width, height,
			params.hasAlpha ? GL_RGBA : GL_RGB,
			GL_UNSIGNED_BYTE,
			pixels
		);

		apply_params(params);

		if (params.genMips)
			glGenerateTextureMipmap(mID);
	}

	void Texture2D::upload(const float* pixels,
		u32 width, u32 height, u32 channels,
		const TextureParams& params) noexcept
	{
		mWidth = width;
		mHeight = height;

		GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;

		glTextureStorage2D(mID,
			params.genMips ? 8 : 1,
			GL_RGB16F, width, height);

		glTextureSubImage2D(
			mID, 0, 0, 0, width, height,
			format, GL_FLOAT, pixels
		);

		apply_params(params);

		if (params.genMips)
			glGenerateTextureMipmap(mID);
	}

	void Texture2D::allocate(
		u32 width, u32 height, u32 internalFormat,
		const TextureParams& params) noexcept
	{
		mWidth = width;
		mHeight = height;

		glTextureStorage2D(mID, params.genMips ? 8 : 1,
			internalFormat, width, height);

		apply_params(params);

		if (params.genMips)
			glGenerateTextureMipmap(mID);
	}

	void Texture2D::bind(u32 unit) const noexcept
	{
		glBindTextureUnit(unit, mID);
	}

	void Texture2D::apply_params(const TextureParams& params) const noexcept
	{
		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<i32>(params.wrapS));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<i32>(params.wrapT));
		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, static_cast<i32>(params.minFilter));
		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, static_cast<i32>(params.magFilter));
	}
}