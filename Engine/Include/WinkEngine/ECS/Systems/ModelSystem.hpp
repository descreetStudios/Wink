#pragma once

#include <WinkEngine/GFX/RES/Handle.hpp>
#include <WinkEngine/ECS/Scene.hpp>

namespace Wink::ECS
{
	EntityID instantiate_model(GFX::RES::ModelHandle handle,
		Scene* scene = nullptr, std::optional<EntityID> root = std::nullopt);
}