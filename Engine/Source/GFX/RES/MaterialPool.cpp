#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/MaterialPool.hpp>

namespace Wink::GFX::RES
{
	MaterialHandle MaterialPool::create(ShaderHandle shader)
	{
		return allocate(shader);
	}

	void MaterialPool::destroy(MaterialHandle handle) noexcept
	{
		deallocate(handle);
	}

	MaterialHandle MaterialPool::clone(MaterialHandle handle)
	{
		const Material* m = try_get(handle);
		if (!m) return {};
		return allocate(m->shader, m->textures, m->params);
	}

	void MaterialPool::apply(MaterialHandle handle) const noexcept
	{
		with(handle, [](Material& m) { m.apply(); });
	}

	bool MaterialPool::is_valid(MaterialHandle handle) const noexcept
	{
		return ResourcePool::is_valid(handle)
			&& get_or(handle, [](Material& m) { return m.is_valid(); });
	}
}