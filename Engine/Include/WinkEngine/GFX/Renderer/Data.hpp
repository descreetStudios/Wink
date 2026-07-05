#pragma once

#include <WinkEngine/GFX/RES/Handle.hpp>

namespace Wink::GFX
{
	inline constexpr u32 MAX_DIR_LIGHTS = 2;
	inline constexpr u32 MAX_POINT_LIGHTS = 16;
	inline constexpr u32 MAX_SPOT_LIGHTS = 8;

	struct DirLight
	{
		glm::vec3 direction = { 0.0f, -1.0f, 0.0f };
		float intensity = 1.0f;
		glm::vec3 color = glm::vec3(1.0f);
	};

	struct PointLight
	{
		glm::vec3 position = glm::vec3(0.0f);
		float intensity = 1.0f;
		glm::vec3 color = glm::vec3(1.0f);
		float radius = 15.0f;
	};

	struct SpotLight
	{
		glm::vec3 position = glm::vec3(0.0f);
		float range = 45.0f;
		glm::vec3 direction = { 0.0f, -1.0f, 0.0f };
		float innerCutoff = glm::cos(glm::radians(15.0f));
		glm::vec3 color = glm::vec3(1.0f);
		float outerCutoff = glm::cos(glm::radians(30.0f));
		float intensity = 1.0f;
	};

	struct RenderObject
	{
		RES::MeshHandle mesh;
		RES::MaterialHandle material;
	};

	struct CameraData
	{
		glm::vec3 position;
		glm::mat4 viewProj;
	};

	struct DrawData
	{
		const RenderObject& renderObj;
		const CameraData& camData;
		const glm::mat4& modelMat;
		const glm::mat3& normalMat;

		std::span<const DirLight> dirLights;
		std::span<const PointLight> pointLights;
		std::span<const SpotLight> spotLights;
	};
}