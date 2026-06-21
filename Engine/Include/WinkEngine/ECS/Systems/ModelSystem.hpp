#pragma once

#include <WinkEngine/GFX/Resource/Handle.hpp>
#include <WinkEngine/ECS/Scene.hpp>

namespace Wink::ECS
{
	EntityID instantiate_model(GFX::Resource::ModelHandle handle,
		Scene* scene = nullptr, std::optional<EntityID> root = std::nullopt);
}