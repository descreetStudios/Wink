#pragma once

namespace Wink::GFX
{
	struct alignas(16) DirLightGPU
	{
		glm::vec4 direction;  // .w = intensity
		glm::vec4 color;      // .w = 0
	};

	struct alignas(16) PointLightGPU
	{
		glm::vec4 position;   // .w = intensity
		glm::vec4 color;      // .w = radius
	};

	struct alignas(16) SpotLightGPU
	{
		glm::vec4 position;   // .w = range
		glm::vec4 direction;  // .w = intensity
		glm::vec4 color;      // .w = outerCutoff
		glm::vec4 inner;      // .x = innerCutoff, .yzw = 0
	};

	struct FrameGPUData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 invProj;
		glm::mat4 viewProj;
		glm::vec3 camPos;		float _pad;
	};

	struct LightsGPUData
	{
		u32 dirLightCount;
		u32 pointLightCount;
		u32 spotLightCount;
		u32 _pad;

		DirLightGPU dirLights[MAX_DIR_LIGHTS];
		PointLightGPU pointLights[MAX_POINT_LIGHTS];
		SpotLightGPU spotLights[MAX_SPOT_LIGHTS];
	};
}