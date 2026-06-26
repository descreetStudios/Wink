#pragma once

namespace Wink::GFX
{
	enum class TextureWrap : i32
	{
		Repeat = GL_REPEAT,
		MirroredRepeat = GL_MIRRORED_REPEAT,
		ClampToEdge = GL_CLAMP_TO_EDGE,
		ClampToBorder = GL_CLAMP_TO_BORDER,
	};

	enum class TextureFilter : i32
	{
		Nearest = GL_NEAREST,
		Linear = GL_LINEAR,
		LinearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR,
		NearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
	};

	struct TextureParams
	{
		TextureWrap wrapS = TextureWrap::Repeat;
		TextureWrap wrapT = TextureWrap::Repeat;
		TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
		TextureFilter magFilter = TextureFilter::Linear;
		bool genMips = true;
		bool sRGB = true;
		bool hasAlpha = true;
	};

	class Texture2D
	{
	public:
		bool hotReloadEnabled = true;

	public:
		Texture2D();
		~Texture2D();

		DISABLE_COPY(Texture2D);
		MOVE_CTOR(Texture2D) noexcept;
		MOVE_ASSIGN(Texture2D) noexcept;

		void upload(const u8* pixels, u32 width, u32 height,
			const TextureParams& params = {}) noexcept;

		void allocate(u32 width, u32 height, u32 internalFormat,
			const TextureParams& params = {}) noexcept;

		void bind(u32 unit = 0) const noexcept;

		[[nodiscard]] u32 get_id() const noexcept { return mID; }
		[[nodiscard]] u32 get_width() const noexcept { return mWidth; }
		[[nodiscard]] u32 get_height() const noexcept { return mHeight; }
		[[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		void apply_params(const TextureParams& params) const noexcept;

	private:
		u32 mID = 0;
		u32 mWidth = 0;
		u32 mHeight = 0;
	};
}