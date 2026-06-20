#pragma once

#include <WinkEngine/GFX/Resource/ResourcePool.hpp>
#include <WinkEngine/GFX/Mesh.hpp>

namespace Wink::GFX::Resource
{
	class MeshPool final : public ResourcePool<Mesh, MeshTag>
	{
	public:
		MeshHandle load(const MeshData& data,
			BufferUsage usage = BufferUsage::StaticDraw);

		void unload(MeshHandle handle) noexcept;

		/* Mesh operations */
		void update(MeshHandle handle, const MeshData& data) const noexcept;

		[[nodiscard]] u32 get_index_count(MeshHandle handle) const noexcept;
		[[nodiscard]] u32 get_vao_id(MeshHandle handle) const noexcept;
		[[nodiscard]] bool is_valid(MeshHandle handle) const noexcept;
	};
}