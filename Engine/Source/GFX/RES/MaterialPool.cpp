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
		if (!m) return MaterialHandle();
		return allocate(m->shader, m->textures, m->params);
	}

	void MaterialPool::apply(Handle<MaterialTag> handle) const noexcept
	{
		const auto* mat = try_get(handle);
		if (!mat) return;

		mat->apply();
	}

	bool MaterialPool::is_valid(MaterialHandle handle) const noexcept
	{
		bool valid = ResourcePool::is_valid(handle);
		with(handle, [&](Material& m) { valid &= m.is_valid(); });
		return valid;
	}
}