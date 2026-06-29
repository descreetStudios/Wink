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
		assert(depth < 256);

		auto e = scene.wrap(id);
		assert(e.is_valid());

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

		t.normalMatrix = glm::mat3(glm::transpose(glm::inverse(glm::mat3(t.worldMatrix))));
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

	void attach_to_parent(Scene& scene, EntityID child, EntityID newParent)
	{
		assert(child != newParent);

		detach_from_parent(scene, child);

		auto childE = scene.wrap(child);
		auto newParentE = scene.wrap(newParent);

		auto* childT = childE.try_get<TransformComponent>();
		assert(childT != nullptr);

		if (newParent == NULL_ENTITY) return;

		auto* parentT = newParentE.try_get<TransformComponent>();
		assert(parentT != nullptr);

		childT->parent = newParent;
		parentT->children.push_back(child);
	}

	void detach_from_parent(Scene& scene, EntityID child)
	{
		auto childE = scene.wrap(child);

		auto* childT = childE.try_get<TransformComponent>();
		if (!childT || childT->parent == NULL_ENTITY) return;

		auto parentE = scene.wrap(childT->parent);

		auto* parentT = parentE.try_get<TransformComponent>();
		if (parentT)
		{
			auto& siblings = parentT->children;
			siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
		}

		childT->parent = NULL_ENTITY;
	}
}