#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/TextureCubemap.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	namespace
	{
		constexpr GLenum FACE_TARGETS[6] =
		{
			GL_TEXTURE_CUBE_MAP_POSITIVE_X,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
		};

		GLsizei calc_mip_levels(GLsizei texWidth, GLsizei texHeight)
		{
			GLsizei levelCount = 1;
			GLsizei longestDim = std::max(texWidth, texHeight);
			while (longestDim > 1) { longestDim >>= 1; ++levelCount; }
			return levelCount;
		}
	} // anonymous namespace

	TextureCubemap::TextureCubemap(const CubemapSpec& spec)
	{
		allocate_storage(spec);
		apply_default_sampler_params(
			spec.minFilter, spec.magFilter, spec.wrapMode);
		if (spec.generateMipmaps)
			generate_mipmaps();
	}

	TextureCubemap::TextureCubemap(
		const std::array<CubemapFaceDesc, 6>& faces,
		TextureFilter minFilter, TextureFilter magFilter,
		bool genMips)
	{
		if (faces[0].width <= 0 || faces[0].height <= 0)
		{
			Logger::Internal::error("Trying to create a cubemap with incorrect data");
			return;
		}

		CubemapSpec spec;
		spec.width = faces[0].width;
		spec.height = faces[0].height;
		spec.internalFormat = faces[0].internalFormat;
		spec.minFilter = minFilter;
		spec.magFilter = magFilter;
		spec.wrapMode = TextureWrap::ClampToEdge;
		spec.generateMipmaps = genMips;
		spec.mipLevels = genMips ?
			calc_mip_levels(spec.width, spec.height) : 1;

		allocate_storage(spec);
		apply_default_sampler_params(
			minFilter, magFilter, TextureWrap::ClampToEdge);

		for (u32 fidx = 0; fidx < 6; ++fidx)
		{
			const auto& faceDesc = faces[fidx];
			if (faceDesc.data)
			{
				upload_face(static_cast<CubemapFace>(FACE_TARGETS[fidx]),
					0, faceDesc.width, faceDesc.height,
					faceDesc.format, faceDesc.dataType, faceDesc.data);
			}
		}

		if (genMips) generate_mipmaps();
	}

	TextureCubemap::~TextureCubemap()
	{
		destroy();
	}

	MOVE_CTOR_IMPL(TextureCubemap) noexcept
		: mID(o.mID)
		, mWidth(o.mWidth)
		, mHeight(o.mHeight)
		, mInternalFormat(o.mInternalFormat)
		, mMipLevels(o.mMipLevels)
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(TextureCubemap) noexcept
	{
		if (this != &o)
		{
			destroy();
			mID = o.mID;
			mWidth = o.mWidth;
			mHeight = o.mHeight;
			mInternalFormat = o.mInternalFormat;
			mMipLevels = o.mMipLevels;
			o.mID = 0;
		}
		return *this;
	}

	void TextureCubemap::upload_face(
		CubemapFace face, u32 mipLevel,
		u32 width, u32 height, u32 format,
		u32 dataType, const void* data) const
	{
		if (!is_valid())
		{
			Logger::Internal::error("Trying to upload a face to an invalid cubemap");
			return;
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, mID);
		glTexSubImage2D(static_cast<GLenum>(face),
			mipLevel, 0, 0, width, height,
			format, dataType, data);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void TextureCubemap::upload_compressed_face(
		CubemapFace face, u32 mipLevel, u32 format,
		u32 width, u32 height, u32 imageSize, const void* data) const
	{
		if (!is_valid())
		{
			Logger::Internal::error("Trying to upload a compressed face to an invalid cubemap");
			return;
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, mID);
		glCompressedTexSubImage2D(
			static_cast<GLenum>(face),
			mipLevel, 0, 0, width, height,
			format, imageSize, data);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void TextureCubemap::set_min_filter(TextureFilter filterMode) const
	{
		if (!is_valid()) 
		{
			Logger::Internal::error("Trying to set min filter to an invalid cubemap");
			return;
		}

		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER,
			static_cast<GLint>(filterMode));
	}

	void TextureCubemap::set_mag_filter(TextureFilter filterMode) const
	{
		if (!is_valid()) 
		{
			Logger::Internal::error("Trying to set mag filter to an invalid cubemap");
			return;
		}

		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER,
			static_cast<GLint>(filterMode));
	}

	void TextureCubemap::set_wrap_mode(TextureWrap wrapMode) const
	{
		set_wrap_mode(wrapMode, wrapMode, wrapMode);
	}

	void TextureCubemap::set_wrap_mode(
		TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR) const
	{
		if (!is_valid())
		{
			Logger::Internal::error("Trying to set wrap mode to an invalid cubemap");
			return;
		}

		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapS));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapT));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_R, static_cast<GLint>(wrapR));
	}

	void TextureCubemap::set_max_anisotropy(float maxAnisotropy) const
	{
		if (!is_valid())
		{
			Logger::Internal::error("Trying to set max anisotropy to an invalid cubemap");
			return;
		}

		glTextureParameterf(mID, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
	}

	void TextureCubemap::generate_mipmaps() const
	{
		if (!is_valid())
		{
			Logger::Internal::error("Trying to generate mipmaps for an invalid cubemap");
			return;
		}

		glGenerateTextureMipmap(mID);
	}

	void TextureCubemap::bind(GLuint textureUnit) const
	{
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mID);
	}

	void TextureCubemap::bind_image(u32 unit, u32 mipLevel,
		bool layered, GLenum access, GLenum format) const
	{
		if (!is_valid())
		{
			Logger::Internal::error("Trying to bind an invalid cubemap as image");
			return;
		}

		glBindImageTexture(unit, mID, mipLevel,
			GL_TRUE, 0, access, format);
	}

	void TextureCubemap::allocate_storage(const CubemapSpec& spec)
	{
		assert(spec.width > 0 && spec.height > 0);

		if (spec.width <= 0 || spec.height <= 0)
		{
			Logger::Internal::error("Trying to allocate cubemap storage with invalid dimensions");
			return;
		}

		mWidth = spec.width;
		mHeight = spec.height;
		mInternalFormat = spec.internalFormat;
		mMipLevels = spec.generateMipmaps
			? calc_mip_levels(spec.width, spec.height)
			: std::max(spec.mipLevels, 1u);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &mID);

		// Allocates all 6 faces at once.
		glTextureStorage2D(mID, mMipLevels,
			mInternalFormat, mWidth, mHeight);
	}

	void TextureCubemap::apply_default_sampler_params(
		TextureFilter minFilter, TextureFilter magFilter,
		TextureWrap wrapMode) const
	{
		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minFilter));
		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(magFilter));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapMode));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapMode));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_R, static_cast<GLint>(wrapMode));
	}

	void TextureCubemap::destroy()
	{
		if (mID != 0)
		{
			glDeleteTextures(1, &mID);
			mID = 0;
		}
	}
}