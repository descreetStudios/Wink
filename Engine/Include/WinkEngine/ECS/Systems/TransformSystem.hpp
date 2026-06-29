#pragma once

#include <WinkEngine/ECS/Entity.hpp>

namespace Wink::ECS
{
	struct TransformComponent;

	[[nodiscard]] glm::mat4 get_local_matrix(const TransformComponent& t);
	void update_world_transform(Scene& scene, EntityID id, u32 depth = 0);
	void reset_transform(TransformComponent& t);
}