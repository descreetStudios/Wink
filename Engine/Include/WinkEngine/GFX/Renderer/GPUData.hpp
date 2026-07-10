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

	struct alignas(16) FrameGPUData
	{
		glm::mat4 view;
		glm::mat4 viewProj;
		glm::vec3 camPos;
		u32 tileCountX;
	};

	struct alignas(16) LightsGPUData
	{
		u32 dirLightCount;
		u32 _pad;
		u32 _pad1;
		u32 _pad2;

		DirLightGPU dirLights[MAX_DIR_LIGHTS];
	};
}