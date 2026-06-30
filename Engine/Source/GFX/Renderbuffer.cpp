#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderbuffer.hpp>

namespace Wink::GFX
{
	Renderbuffer::Renderbuffer()
	{
		glCreateRenderbuffers(1, &mID);
		assert(mID != 0);
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

	void Renderbuffer::allocate(u32 internalFormat,
		u32 width, u32 height, u32 samples) const noexcept
	{
		assert(is_valid());
		assert(width > 0 && height > 0);

		if (samples > 1)
		{
			glNamedRenderbufferStorageMultisample(
				mID, samples, internalFormat,
				width, height
			);
		}
		else
		{
			glNamedRenderbufferStorage(
				mID, internalFormat,
				width, height
			);
		}
	}
}