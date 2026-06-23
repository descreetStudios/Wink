#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Resource/MeshPool.hpp>

namespace Wink::GFX::Resource
{
	MeshHandle MeshPool::load(const MeshData& data,
		BufferUsage usage)
	{
		MeshHandle handle = allocate();

		with(handle, [&](Mesh& mesh)
			{
				mesh.vbo.upload(std::span(data.vertices), usage);
				mesh.ebo.upload(std::span(data.indices), usage);
				mesh.indexCount = static_cast<u32>(data.indices.size());

				mesh.vao.bind_vertex_buffer(
					0, mesh.vbo.get_id(),
					0, sizeof(Vertex));
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
		with(handle, [&](Mesh& mesh)
			{
				mesh.vbo.upload(std::span(data.vertices), BufferUsage::DynamicDraw);
				mesh.ebo.upload(std::span(data.indices), BufferUsage::DynamicDraw);
				mesh.indexCount = static_cast<u32>(data.indices.size());
			});
	}

	u32 MeshPool::get_index_count(MeshHandle handle) const noexcept
	{
		u32 count = 0;
		with(handle, [&](Mesh& mesh) { count = mesh.indexCount; });
		return count;
	}

	u32 MeshPool::get_vao_id(MeshHandle handle) const noexcept
	{
		u32 id = 0;
		with(handle, [&](Mesh& mesh) { id = mesh.vao.get_id(); });
		return id;
	}

	bool MeshPool::is_valid(MeshHandle handle) const noexcept
	{
		bool valid = ResourcePool::is_valid(handle);
		with(handle, [&](Mesh& mesh) {
			valid &= mesh.vao.is_valid() &&
				mesh.vbo.is_valid() &&
				mesh.ebo.is_valid();
			});
		return valid;
	}
}