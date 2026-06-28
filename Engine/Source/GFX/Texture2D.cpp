#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Texture2D.hpp>

namespace Wink::GFX
{
	namespace
	{
		u32 calculate_mip_levels(u32 width, u32 height) noexcept
		{
			return static_cast<u32>(std::floor(
				std::log2(std::max(width, height)))) + 1;
		}

		[[nodiscard]] GLenum texture_data_type_to_gl(TextureDataType type) noexcept
		{
			switch (type)
			{
			case TextureDataType::UnsignedByte: return GL_UNSIGNED_BYTE;
			case TextureDataType::Byte: return GL_BYTE;
			case TextureDataType::UnsignedShort: return GL_UNSIGNED_SHORT;
			case TextureDataType::Short: return GL_SHORT;
			case TextureDataType::UnsignedInt: return GL_UNSIGNED_INT;
			case TextureDataType::Int: return GL_INT;
			case TextureDataType::HalfFloat: return GL_HALF_FLOAT;
			case TextureDataType::Float: return GL_FLOAT;
			}
			return 0;
		}
	} // anonymous namespace

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
		u32 width, u32 height, u32 channels,
		TextureDataType dataType,
		const Texture2DParams& params) noexcept
	{
		mWidth = width;
		mHeight = height;

		GLenum internalFormat = GL_RGBA8;
		GLenum pixelFormat = GL_RGBA;

		switch (channels)
		{
		case 1:
			internalFormat = GL_R8;
			pixelFormat = GL_RED;
			break;
		case 2:
			internalFormat = GL_RG8;
			pixelFormat = GL_RG;
			break;
		case 3:
			internalFormat = params.sRGB ? GL_SRGB8 : GL_RGB8;
			pixelFormat = GL_RGB;
			break;
		case 4:
		default:
			internalFormat = params.sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
			pixelFormat = GL_RGBA;
			break;
		}

		u32 levels = params.genMips ?
			calculate_mip_levels(width, height) : 1;

		glPixelStorei(GL_UNPACK_ALIGNMENT,
			(channels == 1 || channels == 3) ? 1 : 4);

		glTextureStorage2D(static_cast<GLuint>(mID),
			levels, internalFormat, width, height);

		if (pixels)
		{
			glTextureSubImage2D(
				static_cast<GLuint>(mID),
				0, 0, 0, width, height,
				pixelFormat,
				texture_data_type_to_gl(dataType),
				pixels
			);
		}

		apply_params(params);

		if (params.genMips && pixels)
			glGenerateTextureMipmap(static_cast<GLuint>(mID));

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	void Texture2D::upload(const float* pixels,
		u32 width, u32 height, u32 channels,
		TextureDataType dataType,
		const Texture2DParams& params) noexcept
	{
		mWidth = width;
		mHeight = height;

		GLenum internalFormat = GL_RGBA16F;
		GLenum pixelFormat = GL_RGBA;

		switch (channels)
		{
		case 1:
			internalFormat = GL_R16F;
			pixelFormat = GL_RED;
			break;
		case 2:
			internalFormat = GL_RG16F;
			pixelFormat = GL_RG;
			break;
		case 3:
			internalFormat = GL_RGB16F;
			pixelFormat = GL_RGB;
			break;
		case 4:
		default:
			internalFormat = GL_RGBA16F;
			pixelFormat = GL_RGBA;
			break;
		}

		u32 levels = params.genMips ? calculate_mip_levels(width, height) : 1;

		u32 rowStrideBytes = static_cast<unsigned long long>(width)
			* channels * sizeof(float);
		glPixelStorei(GL_UNPACK_ALIGNMENT, (rowStrideBytes % 4 == 0) ? 4 : 1);

		glTextureStorage2D(static_cast<GLuint>(mID),
			levels, internalFormat, width, height);

		if (pixels)
		{
			glTextureSubImage2D(
				static_cast<GLuint>(mID),
				0, 0, 0, width, height,
				pixelFormat,
				texture_data_type_to_gl(dataType),
				pixels
			);
		}

		apply_params(params);

		if (params.genMips && pixels)
			glGenerateTextureMipmap(static_cast<GLuint>(mID));

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	void Texture2D::allocate(
		u32 width, u32 height, u32 internalFormat,
		const Texture2DParams& params) noexcept
	{
		mWidth = width;
		mHeight = height;

		glTextureStorage2D(static_cast<GLuint>(mID),
			params.genMips ? 8 : 1,
			internalFormat, width, height);

		apply_params(params);

		if (params.genMips)
			glGenerateTextureMipmap(static_cast<GLuint>(mID));
	}

	void Texture2D::bind(u32 unit) const noexcept
	{
		glBindTextureUnit(unit, static_cast<GLuint>(mID));
	}

	void Texture2D::apply_params(const Texture2DParams& params) const noexcept
	{
		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<GLint>(params.wrapS));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<GLint>(params.wrapT));
		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(params.minFilter));
		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(params.magFilter));
	}
}