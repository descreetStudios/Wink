#pragma once

namespace Wink::GFX
{
	enum class BufferTarget : u32
	{
		Vertex = GL_ARRAY_BUFFER,
		Index = GL_ELEMENT_ARRAY_BUFFER,
		Uniform = GL_UNIFORM_BUFFER,
		ShaderStore = GL_SHADER_STORAGE_BUFFER,
		DrawIndirect = GL_DRAW_INDIRECT_BUFFER
	};

	enum class BufferUsage : u32
	{
		StaticDraw = GL_STATIC_DRAW,
		DynamicDraw = GL_DYNAMIC_DRAW,
		StreamDraw = GL_STREAM_DRAW,
		StaticRead = GL_STATIC_READ,
		DynamicRead = GL_DYNAMIC_READ,
	};

	class Buffer
	{
	public:
		explicit Buffer(BufferTarget target);
		~Buffer() noexcept;

		DISABLE_COPY(Buffer);
		MOVE_CTOR(Buffer) noexcept;
		MOVE_ASSIGN(Buffer) noexcept;

		void upload(const void* data, size_t size,
			BufferUsage usage = BufferUsage::StaticDraw) noexcept;

		template <typename T>
		void upload(std::span<const T> data,
			BufferUsage usage = BufferUsage::StaticDraw) noexcept
		{
			upload(data.data(), data.size_bytes(), usage);
		}

		// Partial update, must be within already allocated range
		void update(const void* data, size_t size,
			size_t offsetBytes = 0) const noexcept;

		// Partial update, must be within already allocated range
		template <typename T>
		void update(std::span<const T> data,
			size_t offsetBytes = 0) const noexcept
		{
			update(data.data(), data.size_bytes(), offsetBytes);
		}

		void bind_base(u32 bindingPoint) const noexcept;
		void bind_range(u32 bindingPoint, size_t offsetBytes,
			size_t sizeBytes) const noexcept;

		[[nodiscard]] u32 get_id() const noexcept { return mID; }
		[[nodiscard]] size_t get_size() const noexcept { return mSize; }
		[[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		u32 mID = 0;
		u32 mTarget = 0;
		size_t mSize = 0;
	};
}