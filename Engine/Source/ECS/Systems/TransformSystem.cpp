#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::ECS
{
	glm::mat4 get_local_matrix(const TransformComponent& t)
	{
		glm::mat4 m = glm::translate(glm::mat4(1.0f), t.position);
		m *= glm::mat4_cast(t.rotation);
		m = glm::scale(m, t.scale);
		return m;
	}

	void update_world_transform(Scene& scene, EntityID id, u32 depth)
	{
		auto e = scene.wrap(id);
		if (!e.is_valid())
		{
			Logger::Internal::critical("Operating on invalid entity");
			return;
		}

		assert(depth < 256);

		auto* tPtr = e.try_get<TransformComponent>();
		assert(tPtr != nullptr);
		auto& t = *tPtr;

		if (t.parent == NULL_ENTITY)
			t.worldMatrix = get_local_matrix(t);
		else
		{
			const auto& parentT = scene.wrap(t.parent).get<TransformComponent>();
			if (parentT.dirty)
				update_world_transform(scene, t.parent, depth + 1);
			t.worldMatrix = parentT.worldMatrix * get_local_matrix(t);
		}

		t.normalMatrix = glm::mat3(glm::transpose(glm::inverse(t.worldMatrix)));
		t.dirty = false;
	}

	void reset_transform(TransformComponent& t)
	{
		t.position = glm::vec3(0.0f);
		t.rotation = glm::identity<glm::quat>();
		t.scale = glm::vec3(1.0f);
		t.worldMatrix = glm::mat4(1.0f);
		t.normalMatrix = glm::mat3(1.0f);
		t.dirty = true;
	}
}