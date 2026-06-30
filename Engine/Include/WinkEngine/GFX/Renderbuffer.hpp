#pragma once

namespace Wink::GFX
{
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