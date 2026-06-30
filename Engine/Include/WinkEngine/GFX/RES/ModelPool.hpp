#pragma once

#include <WinkEngine/GFX/RES/ResourcePool.hpp>
#include <WinkEngine/Content/Model.hpp>

namespace Wink::GFX::RES
{
	class ModelPool final : public ResourcePool<Content::Model, ModelTag>
	{
	public:
		ModelHandle load(const fs::path& path, ShaderHandle shader = {});
		void unload(ModelHandle handle) noexcept;

		[[nodiscard]] bool is_valid(ModelHandle handle) const noexcept;
	};
}