#pragma once

#include <WinkEngine/GFX/RES/ResourcePool.hpp>
#include <WinkEngine/GFX/Material.hpp>

namespace Wink::GFX::RES
{
	class MaterialPool final : public ResourcePool<Material, MaterialTag>
	{
	public:
		MaterialHandle create(ShaderHandle shader);
		void destroy(MaterialHandle handle) noexcept;
		MaterialHandle clone(MaterialHandle handle);

		/* Material operations */
		void apply(MaterialHandle handle) const noexcept;

		[[nodiscard]] bool is_valid(MaterialHandle handle) const noexcept;
	};
}