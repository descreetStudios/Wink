#pragma once

namespace Wink::GFX
{
	class Texture2D;

	enum class Attachment : u32
	{
		Color0 = GL_COLOR_ATTACHMENT0,
		Color1 = GL_COLOR_ATTACHMENT1,
		Color2 = GL_COLOR_ATTACHMENT2,
		Color3 = GL_COLOR_ATTACHMENT3,
		Depth = GL_DEPTH_ATTACHMENT,
		Stencil = GL_STENCIL_ATTACHMENT,
		DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
	};

	class Framebuffer
	{
	public:
		Framebuffer();
		~Framebuffer();

		DISABLE_COPY(Framebuffer);
		MOVE_CTOR(Framebuffer) noexcept;
		MOVE_ASSIGN(Framebuffer) noexcept;

		void attach(const Texture2D& tex, Attachment slot) const noexcept;
		void attach_renderbuffer(u32 rboID, Attachment slot) const noexcept;

		void set_draw_buffers(std::initializer_list<Attachment> slots) const noexcept;

		[[nodiscard]] bool check() const noexcept;

		void blit_to(u32 dstID,
			u32 srcW, u32 srcH,
			u32 dstW, u32 dstH,
			u32 mask = GL_COLOR_BUFFER_BIT,
			u32 filter = GL_LINEAR) const noexcept;

		[[nodiscard]] u32 get_id() const noexcept { return mID; }
		[[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		u32 mID = 0;
	};

	class Renderbuffer
	{
	public:
		Renderbuffer();
		~Renderbuffer();

		DISABLE_COPY(Renderbuffer);
		MOVE_CTOR(Renderbuffer) noexcept;
		MOVE_ASSIGN(Renderbuffer) noexcept;

		void allocate(u32 internalFormat,
			u32 width, u32 height,
			u32 samples = 1) const noexcept;

		[[nodiscard]] u32 get_id() const noexcept { return mID; }
		[[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		u32 mID = 0;
	};
}