#pragma once

namespace Wink::GFX
{
	class VAO
	{
	public:
		VAO();
		~VAO() noexcept;

		DISABLE_COPY(VAO);
		MOVE_CTOR(VAO) noexcept;
		MOVE_ASSIGN(VAO) noexcept;

		void attrib(u32 index, i32 count, u32 type,
			i32 stride, size_t offset,
			bool normalized = false) const noexcept;

		void attrib_i(u32 index, i32 count, u32 type,
			i32 stride, size_t offset) const noexcept;

		void divisor(u32 index, u32 div) const noexcept;

		void bind_vertex_buffer(
			u32 bindingIndex, u32 bufferID,
			size_t offset, i32 stride) const noexcept;

		void bind_index_buffer(u32 bufferID) const noexcept;

		void reset() noexcept;
		void label(const char* name) const noexcept;

		[[nodiscard]] u32 get_id() const noexcept { return mID; }
		[[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		u32 mID = 0;
	};
}