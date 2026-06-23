#pragma once

namespace Wink::ECS
{
	struct DirLightComponent
	{
		glm::vec3 direction = { 0.0f, -1.0f, 0.0f };
		float intensity = 1.0f;
		glm::vec3 color = glm::vec3(1.0f);
	};
}