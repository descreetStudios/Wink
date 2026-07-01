#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/MeshPool.hpp>

namespace Wink::GFX::RES
{
	MeshHandle MeshPool::load(const MeshData& data, BufferUsage usage)
	{
		assert(!data.vertices.empty());
		assert(!data.indices.empty());

		MeshHandle handle = allocate();

		with(handle, [&](Mesh& mesh)
			{
				mesh.vbo.upload(std::span(data.vertices), usage);
				mesh.ebo.upload(std::span(data.indices), usage);
				mesh.indexCount = static_cast<u32>(data.indices.size());

				mesh.vao.bind_vertex_buffer(0, mesh.vbo.get_id(), 0, sizeof(Vertex));
				mesh.vao.bind_index_buffer(mesh.ebo.get_id());

				mesh.vao.attrib(0, 3, GL_FLOAT, offsetof(Vertex, position));
				mesh.vao.attrib(1, 3, GL_FLOAT, offsetof(Vertex, normal));
				mesh.vao.attrib(2, 2, GL_FLOAT, offsetof(Vertex, uv));
				mesh.vao.attrib(3, 2, GL_FLOAT, offsetof(Vertex, uv1));
				mesh.vao.attrib(4, 4, GL_FLOAT, offsetof(Vertex, tangent));
			});

		return handle;
	}

	void MeshPool::unload(MeshHandle handle) noexcept
	{
		deallocate(handle);
	}

	void MeshPool::update(MeshHandle handle, const MeshData& data) const noexcept
	{
		assert(!data.vertices.empty());
		assert(!data.indices.empty());

		with(handle, [&](Mesh& mesh)
			{
				mesh.vbo.upload(std::span(data.vertices), BufferUsage::DynamicDraw);
				mesh.ebo.upload(std::span(data.indices), BufferUsage::DynamicDraw);
				mesh.indexCount = static_cast<u32>(data.indices.size());
			});
	}

	u32 MeshPool::get_index_count(MeshHandle handle) const noexcept
	{
		return get_or(handle, [](Mesh& mesh) { return mesh.indexCount; });
	}

	u32 MeshPool::get_vao_id(MeshHandle handle) const noexcept
	{
		return get_or(handle, [](Mesh& mesh) { return mesh.vao.get_id(); });
	}

	bool MeshPool::is_valid(MeshHandle handle) const noexcept
	{
		return ResourcePool::is_valid(handle)
			&& get_or(handle, [](Mesh& mesh) {
			return mesh.vao.is_valid()
				&& mesh.vbo.is_valid()
				&& mesh.ebo.is_valid();
				});
	}
}