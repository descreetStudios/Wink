#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/VAO.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	VAO::VAO()
	{
		glCreateVertexArrays(1, &mID);
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
		size_t offset, u32 bindingIndex,
		bool normalized) const noexcept
	{
		assert(is_valid());
		assert(offset <= UINT32_MAX);

		glEnableVertexArrayAttrib(mID, index);
		glVertexArrayAttribFormat(mID, index, count, type,
			static_cast<GLboolean>(normalized),
			static_cast<u32>(offset));
		glVertexArrayAttribBinding(mID, index, bindingIndex);
	}

	void VAO::attrib_i(u32 index, i32 count,
		u32 type, size_t offset,
		u32 bindingIndex) const noexcept
	{
		assert(is_valid());
		assert(offset <= UINT32_MAX);

		glEnableVertexArrayAttrib(mID, index);
		glVertexArrayAttribIFormat(mID, index, count, type,
			static_cast<u32>(offset));
		glVertexArrayAttribBinding(mID, index, bindingIndex);
	}

	void VAO::divisor(u32 index, u32 div) const noexcept
	{
		assert(is_valid());
		glVertexArrayBindingDivisor(mID, index, div);
	}

	void VAO::bind_vertex_buffer(
		u32 bindingIndex, u32 bufferID,
		size_t offset, i32 stride) const noexcept
	{
		assert(is_valid());
		assert(stride >= 0);

		glVertexArrayVertexBuffer(mID, bindingIndex, bufferID,
			static_cast<GLintptr>(offset), stride);
	}

	void VAO::bind_index_buffer(u32 bufferID) const noexcept
	{
		assert(is_valid());
		glVertexArrayElementBuffer(mID, bufferID);
	}

	void VAO::reset() noexcept
	{
		glDeleteVertexArrays(1, &mID);
		glCreateVertexArrays(1, &mID);
	}

	void VAO::label(const char* name) const noexcept
	{
		assert(is_valid());
		assert(name != nullptr);
		glObjectLabel(GL_VERTEX_ARRAY, mID, -1, name);
	}
}