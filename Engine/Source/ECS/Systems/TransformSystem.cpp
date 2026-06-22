#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::ECS
{
	glm::mat4 get_local_matrix(const TransformComponent& t)
	{
		const glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);
		const glm::mat4 R = glm::mat4_cast(t.rotation);
		const glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);
		return T * R * S;
	}

	void update_world_transform(Scene& scene, EntityID id)
	{
		auto e = scene.wrap(id);
		if (!e.is_valid())
		{
			Logger::Internal::critical("Operating on invalid entity");
			return;
		}

		auto& t = e.get<TransformComponent>();

		if (t.parent == NULL_ENTITY)
			t.worldMatrix = get_local_matrix(t);
		else
		{
			update_world_transform(scene, t.parent);
			const auto& parentT = scene.wrap(t.parent).get<TransformComponent>();
			t.worldMatrix = parentT.worldMatrix * get_local_matrix(t);
		}

		t.normalMatrix = glm::mat3(glm::transpose(glm::inverse(t.worldMatrix)));
		t.dirty = false;
	}
}