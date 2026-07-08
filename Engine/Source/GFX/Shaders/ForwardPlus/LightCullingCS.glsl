#version 460 core

#include "../Light.glsl"

#define TILE_SIZE 16
#define GROUP_SIZE (TILE_SIZE * TILE_SIZE)
#define UINT_MAX 0xFFFFFFFFu

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

/* --- Uniforms --- */
uniform mat4 uView;
uniform mat4 uInvProj;
uniform uint uScreenWidth;
uniform uint uScreenHeight;
uniform uint uTileCountX;
uniform uint uPointLightCount;
uniform uint uSpotLightCount;
uniform sampler2D uDepth;

/* --- SSBOs --- */
layout(std430, binding = 0) readonly buffer PointLightBuffer
{
	PointLight uPointLights[];
};

layout(std430, binding = 1) readonly buffer SpotLightBuffer
{
	SpotLight uSpotLights[];
};

layout(std430, binding = 2) writeonly buffer LightIndexListBuffer
{
	uint oLightIndexList[];
};

layout(std430, binding = 3) buffer LightGridBuffer
{
	uvec2 oLightGrid[]; // .x = base offset, .y = (pointCount << 16) | spotCount
};

layout(std430, binding = 4) buffer GlobalLightCountBuffer
{
	uint oGlobalLightCount;
};

/* --- Shared Memory --- */
shared uint sMinDepthInt;
shared uint sMaxDepthInt;
shared uint sPointLightCount;
shared uint sSpotLightCount;
shared uint sBaseOffset;

/* --- Helpers --- */
vec3 uv_to_view(vec2 uv, float depth)
{
	vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 view = uInvProj * ndc;
	return view.xyz / view.w;
}

vec4 compute_plane(vec3 p0, vec3 p1, vec3 p2)
{
	vec3 n = normalize(cross(p1 - p0, p2 - p0));
	return vec4(n, -dot(n, p0));
}

bool sphere_intersects_frustum(
	vec3 center, float radius, vec4 planes[4])
{
	if (dot(planes[0].xyz, center) + planes[0].w < -radius) return false;
	if (dot(planes[1].xyz, center) + planes[1].w < -radius) return false;
	if (dot(planes[2].xyz, center) + planes[2].w < -radius) return false;
	if (dot(planes[3].xyz, center) + planes[3].w < -radius) return false;

	return true;
}

bool spot_intersects_frustum(vec3 posVS, vec3 dirVS,
	float range, float outerCutoff, vec4 planes[4])
{
	float sinAngle = sqrt(1.0 - outerCutoff * outerCutoff);
	float cosAngle = outerCutoff;

	vec3 center;
	float radius;

	if (cosAngle >= sinAngle)
	{
		center = posVS;
		radius = range;
	}
	else
	{
		center = posVS + dirVS * range * cosAngle;
		radius = range * sinAngle;
	}

	return sphere_intersects_frustum(center, radius, planes);
}

float linearize_depth(float depth)
{
	float ndcZ = depth * 2.0 - 1.0;
	vec4 view = uInvProj * vec4(0.0, 0.0, ndcZ, 1.0);
	return view.z / view.w;
}

void main()
{
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	uint flatThread = gl_LocalInvocationIndex;

	if (flatThread == 0u)
	{
		sMinDepthInt = UINT_MAX;
		sMaxDepthInt = 0u;
		sPointLightCount = 0u;
		sSpotLightCount = 0u;
	}
	barrier();

	ivec2 pixelCoord = clamp(
		tileID * TILE_SIZE + ivec2(gl_LocalInvocationID.xy),
		ivec2(0), ivec2(uScreenWidth - 1, uScreenHeight - 1));

	vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(uScreenWidth, uScreenHeight);
	float depth = texture(uDepth, uv).r;

	atomicMin(sMinDepthInt, floatBitsToUint(depth));
	atomicMax(sMaxDepthInt, floatBitsToUint(depth));
	barrier();

	float minDepth = uintBitsToFloat(sMinDepthInt);
	float maxDepth = uintBitsToFloat(sMaxDepthInt);

	vec2 tileMin = vec2(tileID) / vec2(uTileCountX, gl_NumWorkGroups.y);
	vec2 tileMax = vec2(tileID + 1) / vec2(uTileCountX, gl_NumWorkGroups.y);
	float minZ = linearize_depth(minDepth);
	float maxZ = linearize_depth(maxDepth);

	vec3 corners[4];
	corners[0] = uv_to_view(vec2(tileMin.x, tileMin.y), 0.0);
	corners[1] = uv_to_view(vec2(tileMax.x, tileMin.y), 0.0);
	corners[2] = uv_to_view(vec2(tileMax.x, tileMax.y), 0.0);
	corners[3] = uv_to_view(vec2(tileMin.x, tileMax.y), 0.0);

	const vec3 origin = vec3(0.0);
	vec4 planes[4];
	planes[0] = compute_plane(origin, corners[1], corners[0]); // bottom
	planes[1] = compute_plane(origin, corners[3], corners[2]); // top
	planes[2] = compute_plane(origin, corners[0], corners[3]); // left
	planes[3] = compute_plane(origin, corners[2], corners[1]); // right

	/* --- Count visible lights --- */
	const mat3 viewRot = mat3(uView);
	for (uint i = flatThread; i < uPointLightCount; i += GROUP_SIZE)
	{
		const PointLight light = uPointLights[i];
		vec3 posVS = (uView * vec4(light.position.xyz, 1.0)).xyz;
		float radius = light.color.w;

		if (posVS.z - radius > minZ) continue;
		if (posVS.z + radius < maxZ) continue;
		if (sphere_intersects_frustum(posVS, radius, planes))
			atomicAdd(sPointLightCount, 1u);
	}

	for (uint i = flatThread; i < uSpotLightCount; i += GROUP_SIZE)
	{
		const SpotLight light = uSpotLights[i];
		vec3 posVS = (uView * vec4(light.position.xyz,1)).xyz;
		vec3 dirVS = normalize(viewRot * light.direction.xyz);
		float range = light.position.w;
		float outerCutoff = light.color.w;

		if (posVS.z - range > minZ) continue;
		if (posVS.z + range < maxZ) continue;
		if (spot_intersects_frustum(posVS, dirVS, range, outerCutoff, planes))
			atomicAdd(sSpotLightCount, 1u);
	}
	barrier();

	// Reserve a contiguous block in the global index list.
	if (flatThread == 0u)
	{
		uint totalLights = sPointLightCount + sSpotLightCount;
		sBaseOffset = atomicAdd(oGlobalLightCount, totalLights);
		sPointLightCount = 0u;
        sSpotLightCount = 0u;
	}
	barrier();

	/* --- Write indices into the reserved block --- */
	for (uint i = flatThread; i < uPointLightCount; i += GROUP_SIZE)
	{
		const PointLight light = uPointLights[i];
		vec3 posVS = (uView * vec4(light.position.xyz, 1.0)).xyz;
		float radius = light.color.w;

		if (posVS.z - radius > minZ) continue;
		if (posVS.z + radius < maxZ) continue;
		if (sphere_intersects_frustum(posVS, radius, planes))
		{
			uint slot = atomicAdd(sPointLightCount, 1u);
			oLightIndexList[sBaseOffset + slot] = i;
		}
	}
	barrier();

	for (uint i = flatThread; i < uSpotLightCount; i += GROUP_SIZE)
	{
		const SpotLight light = uSpotLights[i];
		vec3 posVS = (uView * vec4(light.position.xyz, 1.0)).xyz;
		vec3 dirVS = normalize(viewRot * light.direction.xyz);
		float range = light.position.w;
		float outerCutoff = light.color.w;

		if (posVS.z - range > minZ) continue;
		if (posVS.z + range < maxZ) continue;
		if (spot_intersects_frustum(posVS, dirVS, range, outerCutoff, planes))
		{
			uint slot = atomicAdd(sSpotLightCount, 1u);
			oLightIndexList[sBaseOffset + sPointLightCount + slot] = i;
		}
	}
	barrier();

	/* --- Write light grid entry for this tile --- */
	if (flatThread == 0u)
	{
		uint tileIndex = tileID.y * uTileCountX + tileID.x;

		oLightGrid[tileIndex] = uvec2(sBaseOffset,
			(sPointLightCount << 16) | sSpotLightCount);
	}
}