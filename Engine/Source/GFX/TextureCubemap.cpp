#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/TextureCubemap.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	namespace
	{
		GLsizei calc_mip_levels(GLsizei width, GLsizei height) noexcept
		{
			GLsizei levels = 1;
			GLsizei dim = std::max(width, height);
			while (dim > 1) { dim >>= 1; ++levels; }
			return levels;
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
		TextureFilter minFilter, TextureFilter magFilter, bool genMips)
	{
		assert(faces[0].width > 0 && faces[0].height > 0);

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
		apply_default_sampler_params(minFilter,
			magFilter, TextureWrap::ClampToEdge);

		for (u32 i = 0; i < 6; ++i)
		{
			const auto& face = faces[i];
			if (face.data)
				upload_face(static_cast<CubemapFace>(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
					0, face.width, face.height,
					face.format, face.dataType, face.data);
		}

		if (genMips)
			generate_mipmaps();
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
		o.mWidth = 0;
		o.mHeight = 0;
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
			o.mWidth = 0;
			o.mHeight = 0;
		}
		return *this;
	}

	void TextureCubemap::upload_face(CubemapFace face, u32 mipLevel,
		u32 width, u32 height, u32 format, u32 dataType, const void* data) const
	{
		assert(is_valid());
		assert(data != nullptr);

		glBindTexture(GL_TEXTURE_CUBE_MAP, mID);
		glTexSubImage2D(static_cast<GLenum>(face),
			mipLevel, 0, 0, width, height, format, dataType, data);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void TextureCubemap::upload_compressed_face(CubemapFace face, u32 mipLevel,
		u32 format, u32 width, u32 height, u32 imageSize, const void* data) const
	{
		assert(is_valid());
		assert(data != nullptr);

		glBindTexture(GL_TEXTURE_CUBE_MAP, mID);
		glCompressedTexSubImage2D(static_cast<GLenum>(face),
			mipLevel, 0, 0, width, height, format, imageSize, data);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void TextureCubemap::set_min_filter(TextureFilter filterMode) const
	{
		assert(is_valid());
		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filterMode));
	}

	void TextureCubemap::set_mag_filter(TextureFilter filterMode) const
	{
		assert(is_valid());
		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filterMode));
	}

	void TextureCubemap::set_wrap_mode(TextureWrap wrapMode) const
	{
		set_wrap_mode(wrapMode, wrapMode, wrapMode);
	}

	void TextureCubemap::set_wrap_mode(TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR) const
	{
		assert(is_valid());
		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapS));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapT));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_R, static_cast<GLint>(wrapR));
	}

	void TextureCubemap::set_max_anisotropy(float maxAnisotropy) const
	{
		assert(is_valid());
		glTextureParameterf(mID, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
	}

	void TextureCubemap::generate_mipmaps() const
	{
		assert(is_valid());
		glGenerateTextureMipmap(mID);
	}

	void TextureCubemap::bind(u32 unit) const
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mID);
	}

	void TextureCubemap::bind_image(u32 unit, u32 mipLevel,
		bool layered, GLenum access, GLenum format) const
	{
		assert(is_valid());
		glBindImageTexture(unit, mID, mipLevel, GL_TRUE, 0, access, format);
	}

	void TextureCubemap::allocate_storage(const CubemapSpec& spec)
	{
		assert(spec.width > 0 && spec.height > 0);

		mWidth = spec.width;
		mHeight = spec.height;
		mInternalFormat = spec.internalFormat;
		mMipLevels = spec.generateMipmaps
			? static_cast<u32>(calc_mip_levels(spec.width, spec.height))
			: std::max(spec.mipLevels, 1u);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &mID);
		glTextureStorage2D(mID, mMipLevels, mInternalFormat, mWidth, mHeight);
	}

	void TextureCubemap::apply_default_sampler_params(
		TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrapMode) const
	{
		glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minFilter));
		glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(magFilter));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapMode));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapMode));
		glTextureParameteri(mID, GL_TEXTURE_WRAP_R, static_cast<GLint>(wrapMode));
	}

	void TextureCubemap::destroy()
	{
		glDeleteTextures(1, &mID);
		mID = 0;
	}
}