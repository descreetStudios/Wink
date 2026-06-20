#pragma once

namespace Wink::ECS
{
	class Scene; class Entity;
	struct TransformComponent;

	[[nodiscard]] glm::mat4 get_local_matrix(const TransformComponent& t);
	void update_world_transform(Scene& scene, EntityID id);
}