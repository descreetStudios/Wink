#pragma once

#include <WinkEngine/GFX/Texture2D.hpp>

namespace Wink::GFX
{
	enum class CubemapFace : u32
	{
        PositiveX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        NegativeX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        PositiveY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        NegativeY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        PositiveZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        NegativeZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

    struct CubemapFaceDesc
    {
        u32 internalFormat = GL_RGBA8;
        u32 format = GL_RGBA;
        u32 dataType = GL_UNSIGNED_BYTE;
        u32 width = 0;
        u32 height = 0;
        const void* data = nullptr;
    };

    struct CubemapSpec
    {
        u32 width = 512;
        u32 height = 512;
        u32 internalFormat = GL_RGBA8;
        TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
        TextureFilter magFilter = TextureFilter::Linear;
        TextureWrap wrapMode = TextureWrap::ClampToEdge;
        bool generateMipmaps = false;
        u32 mipLevels = 1;
    };

    class TextureCubemap
    {
    public:
        TextureCubemap() = default;
        explicit TextureCubemap(const CubemapSpec& spec);
        explicit TextureCubemap(const std::array<CubemapFaceDesc, 6>& faces,
            TextureFilter minFilter = TextureFilter::Linear,
            TextureFilter magFilter = TextureFilter::Linear,
            bool generateMipmaps = false);

        ~TextureCubemap();

        DISABLE_COPY(TextureCubemap);
        MOVE_CTOR(TextureCubemap) noexcept;
        MOVE_ASSIGN(TextureCubemap) noexcept;

        void upload_face(CubemapFace face, u32 mipLevel,
            u32 width, u32 height,  u32 format,
            u32 dataType, const void* data) const;

        void upload_compressed_face(
            CubemapFace face, u32 mipLevel, u32 format,
            u32 width, u32 height, u32 imageSize,
            const void* data) const;

        void generate_mipmaps() const;

        void set_min_filter(TextureFilter filterMode) const;
        void set_mag_filter(TextureFilter filterMode) const;
        void set_wrap_mode(TextureWrap wrapMode) const;
        void set_wrap_mode(TextureWrap wrapS,
            TextureWrap wrapT, TextureWrap wrapR) const;
        void set_max_anisotropy(float maxAnisotropy) const;

        void bind(u32 unit = 0) const;
        void bind_image(u32 unit, u32 mipLevel,
            bool layered, GLenum access, GLenum format) const;

        [[nodiscard]] u32 get_id() const noexcept { return mID; }
        [[nodiscard]] u32 get_width() const noexcept { return mWidth; }
        [[nodiscard]] u32 get_height() const noexcept { return mHeight; }
        [[nodiscard]] u32 get_internal_format() const noexcept { return mInternalFormat; }
        [[nodiscard]] u32 get_mip_levels() const noexcept { return mMipLevels; }
        [[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

        explicit operator bool() const noexcept { return is_valid(); }

    private:
        void allocate_storage(const CubemapSpec& spec);
        void apply_default_sampler_params(TextureFilter minFilter,
            TextureFilter magFilter, TextureWrap wrapMode) const;
        void destroy();

        u32 mID = 0;
        u32 mWidth = 0;
        u32 mHeight = 0;
        u32 mInternalFormat = GL_RGBA8;
        u32 mMipLevels = 1;
    };
}