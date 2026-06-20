#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/VAO.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	VAO::VAO()
	{
		glGenVertexArrays(1, &mID);
	}

	VAO::~VAO() noexcept
	{
		glDeleteVertexArrays(1, &mID);
	}

	MOVE_CTOR_IMPL(VAO) noexcept
		: mID(o.mID)
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(VAO) noexcept
	{
		if (this != &o)
		{
			glDeleteVertexArrays(1, &mID);
			mID = o.mID;
			o.mID = 0;
		}
		return *this;
	}

	void VAO::attrib(u32 index, i32 count, u32 type,
		i32 stride, size_t offset, bool normalized) const noexcept
	{
		if (!is_valid())
		{
			Logger::Internal::error(
				"Trying to set vertex attributes on an invalid VAO");
			return;
		}

		glEnableVertexArrayAttrib(mID, index);
		glVertexArrayAttribFormat(
			mID, index, count, type,
			normalized ? GL_TRUE : GL_FALSE,
			static_cast<u32>(offset));

		glVertexArrayAttribBinding(mID, index, index);
	}

	void VAO::attrib_i(u32 index, i32 count, u32 type,
		i32 stride, size_t offset) const noexcept
	{
		if (!is_valid())
		{
			Logger::Internal::error(
				"Trying to set integer vertex attributes on an invalid VAO");
			return;
		}

		glEnableVertexArrayAttrib(mID, index);
		glVertexArrayAttribIFormat(
			mID, index, count, type,
			static_cast<u32>(offset));

		glVertexArrayAttribBinding(mID, index, index);
	}

	void VAO::divisor(u32 index, u32 div) const noexcept
	{
		glVertexArrayBindingDivisor(mID, index, div);
	}

	void VAO::bind_vertex_buffer(
		u32 bindingIndex, u32 bufferID,
		size_t offset, i32 stride) const noexcept
	{
		glVertexArrayVertexBuffer(
			mID, bindingIndex, bufferID,
			static_cast<GLintptr>(offset),
			stride);
	}

	void VAO::bind_index_buffer(u32 bufferID) const noexcept
	{
		glVertexArrayElementBuffer(mID, bufferID);
	}

	void VAO::reset() noexcept
	{
		if (mID) glDeleteVertexArrays(1, &mID);
		glCreateVertexArrays(1, &mID);
	}

	void VAO::label(const char* name) const noexcept
	{
		if (!is_valid())
		{
			Logger::Internal::error(
				"Trying to set an object label to an invalid VAO");
			return;
		}

		glObjectLabel(GL_VERTEX_ARRAY, mID, -1, name);
	}
}