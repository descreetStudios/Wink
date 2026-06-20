#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Buffer.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	Buffer::Buffer(BufferTarget target)
		: mTarget(static_cast<u32>(target))
	{
		glCreateBuffers(1, &mID);
	}

	Buffer::~Buffer() noexcept
	{
		glDeleteBuffers(1, &mID);
	}

	MOVE_CTOR_IMPL(Buffer) noexcept
		: mID(o.mID), mTarget(o.mTarget), mSize(o.mSize)
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(Buffer) noexcept
	{
		if (this != &o)
		{
			glDeleteBuffers(1, &mID);
			mID = o.mID;
			mTarget = o.mTarget;
			mSize = o.mSize;
			o.mID = 0;
		}
		return *this;
	}

	void Buffer::upload(const void* data,
		size_t size, BufferUsage usage) noexcept
	{
		if (!is_valid())
		{
			Logger::Internal::error(
				"Trying to upload data to an invalid buffer");
			return;
		}

		glNamedBufferData(
			mID, static_cast<GLsizeiptr>(size),
			data, static_cast<u32>(usage));

		mSize = size;
	}

	void Buffer::update(const void* data, size_t size,
		size_t offsetBytes) const noexcept
	{
		if (!is_valid())
		{
			Logger::Internal::error(
				"Trying to partially upload data to an invalid buffer");
			return;
		}

		glNamedBufferSubData(
			mID, static_cast<GLintptr>(offsetBytes),
			static_cast<GLsizeiptr>(size), data);
	}

	void Buffer::bind_base(u32 bindingPoint) const noexcept
	{
		if (!is_valid())
		{
			Logger::Internal::error(
				"Trying to bind base of an invalid buffer");
			return;
		}

		glBindBufferBase(mTarget, bindingPoint, mID);
	}

	void Buffer::bind_range(u32 bindingPoint,
		size_t offsetBytes,
		size_t sizeBytes) const noexcept
	{
		if (!is_valid())
		{
			Logger::Internal::error(
				"Trying to bind range of an invalid buffer");
			return;
		}

		glBindBufferRange(mTarget, bindingPoint, mID,
			static_cast<GLintptr>(offsetBytes),
			static_cast<GLsizeiptr>(sizeBytes));
	}
}