#pragma once

#include <WinkEngine/ECS/Entity.hpp>

namespace Wink::ECS
{
	struct TransformComponent
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::identity<glm::quat>();
		glm::vec3 scale = glm::vec3(1.0f);

		glm::mat4 worldMatrix = glm::mat4(1.0f);
		glm::mat3 normalMatrix = glm::mat3(1.0f);
		bool dirty = true;

		EntityID parent = NULL_ENTITY;
	};
}