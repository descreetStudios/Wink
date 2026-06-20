#include <WinkEngine/pch.hpp>
#include <GFX/Framebuffer.hpp>
#include <WinkEngine/GFX/Texture.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	Framebuffer::Framebuffer()
	{
		glCreateFramebuffers(1, &mID);
	}

	Framebuffer::~Framebuffer()
	{
		glDeleteFramebuffers(1, &mID);
	}

	MOVE_CTOR_IMPL(Framebuffer) noexcept
		: mID(o.mID)
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(Framebuffer) noexcept
	{
		if (this != &o)
		{
			glDeleteFramebuffers(1, &mID);
			mID = o.mID;
			o.mID = 0;
		}
		return *this;
	}

	void Framebuffer::attach(
		const Texture2D& tex,
		Attachment slot) const noexcept
	{
		glNamedFramebufferTexture(
			mID,
			static_cast<u32>(slot),
			tex.get_id(),
			0
		);
	}

	void Framebuffer::attach_renderbuffer(
		u32 rboID, Attachment slot) const noexcept
	{
		glNamedFramebufferRenderbuffer(
			mID,
			static_cast<u32>(slot),
			GL_RENDERBUFFER,
			rboID
		);
	}

	void Framebuffer::set_draw_buffers(
		std::initializer_list<Attachment> slots) const noexcept
	{
		std::vector<u32> bufs;
		bufs.reserve(slots.size());

		for (auto s : slots)
			bufs.push_back(static_cast<u32>(s));

		glNamedFramebufferDrawBuffers(
			mID,
			static_cast<i32>(bufs.size()),
			bufs.data()
		);
	}

	bool Framebuffer::check() const noexcept
	{
		u32 status = glCheckNamedFramebufferStatus(mID, GL_FRAMEBUFFER);

		if (status == GL_FRAMEBUFFER_COMPLETE)
			return true;

		Logger::Internal::error("Framebuffer incomplete: 0x{:X}", status);
		return false;
	}

	void Framebuffer::blit_to(u32 dstID,
		u32 srcW, u32 srcH,
		u32 dstW, u32 dstH,
		u32 mask, u32 filter) const noexcept
	{
		glBlitNamedFramebuffer(
			mID, dstID,
			0, 0, srcW, srcH,
			0, 0, dstW, dstH,
			mask, filter
		);
	}

	Renderbuffer::Renderbuffer()
	{
		glCreateRenderbuffers(1, &mID);
	}

	Renderbuffer::~Renderbuffer()
	{
		glDeleteRenderbuffers(1, &mID);
	}

	MOVE_CTOR_IMPL(Renderbuffer) noexcept
		: mID(o.mID)
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(Renderbuffer) noexcept
	{
		if (this != &o)
		{
			glDeleteRenderbuffers(1, &mID);
			mID = o.mID;
			o.mID = 0;
		}
		return *this;
	}

	void Renderbuffer::allocate(
		u32 internalFormat,
		u32 width, u32 height,
		u32 samples) const noexcept
	{
		if (samples > 1)
		{
			glNamedRenderbufferStorageMultisample(
				mID,
				samples,
				internalFormat,
				width,
				height
			);
		}
		else
		{
			glNamedRenderbufferStorage(
				mID,
				internalFormat,
				width,
				height
			);
		}
	}
}