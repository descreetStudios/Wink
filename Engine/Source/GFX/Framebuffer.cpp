#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Framebuffer.hpp>
#include <WinkEngine/GFX/Renderbuffer.hpp>
#include <WinkEngine/GFX/Texture2D.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	Framebuffer::Framebuffer()
	{
		glCreateFramebuffers(1, &mID);
		assert(mID != 0);
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
		assert(is_valid());

		glNamedFramebufferTexture(
			mID,
			static_cast<u32>(slot),
			tex.get_id(),
			0
		);
	}

	void Framebuffer::attach_renderbuffer(
		const Renderbuffer& rbo, Attachment slot) const noexcept
	{
		assert(is_valid());

		glNamedFramebufferRenderbuffer(
			mID,
			static_cast<u32>(slot),
			GL_RENDERBUFFER,
			rbo.get_id()
		);
	}

	void Framebuffer::set_draw_buffers(
		std::initializer_list<Attachment> slots) const noexcept
	{
		assert(slots.size() <= MAX_DRAW_BUFFERS);

		std::array<u32, MAX_DRAW_BUFFERS> bufs{};
		u32 count = 0;

		for (auto s : slots)
			bufs[count++] = static_cast<u32>(s);

		glNamedFramebufferDrawBuffers(
			mID,
			static_cast<i32>(count),
			bufs.data()
		);
	}

	bool Framebuffer::check() const noexcept
	{
		assert(is_valid());

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
}