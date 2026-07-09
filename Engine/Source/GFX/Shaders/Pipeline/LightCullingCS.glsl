#version 460 core

#include "../Light.glsl"

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

layout(std430, binding = 3) coherent buffer LightGridBuffer
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

vec3 compute_plane_normal(vec3 p1, vec3 p2)
{
    return normalize(cross(p1, p2));
}

bool sphere_intersects_frustum(vec3 center, float radius, vec3 normals[4])
{
    if (dot(normals[0], center) < -radius) return false;
    if (dot(normals[1], center) < -radius) return false;
    if (dot(normals[2], center) < -radius) return false;
    if (dot(normals[3], center) < -radius) return false;
    return true;
}

bool spot_intersects_frustum(vec3 posVS, vec3 dirVS,
	float range, float outerCutoff, vec3 normals[4])
{
	float cosA = outerCutoff;
    float sinA = sqrt(1.0 - cosA * cosA);

	float t = step(sinA, cosA);
    vec3 center = mix(posVS + dirVS * (range * cosA), posVS, t);
    float radius = mix(range * sinA, range, t);

    return sphere_intersects_frustum(center, radius, normals);
}

float linearize_depth(float depth)
{
    float ndcZ = depth * 2.0 - 1.0;
    float vz = uInvProj[2][2] * ndcZ + uInvProj[3][2];
    float vw = uInvProj[2][3] * ndcZ + uInvProj[3][3];
    return vz / vw;
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

	float minZ = linearize_depth(uintBitsToFloat(sMinDepthInt));
    float maxZ = linearize_depth(uintBitsToFloat(sMaxDepthInt));

	vec2 tileMin = vec2(tileID) / vec2(uTileCountX, gl_NumWorkGroups.y);
    vec2 tileMax = vec2(tileID + 1) / vec2(uTileCountX, gl_NumWorkGroups.y);

	vec3 corners[4];
    corners[0] = uv_to_view(vec2(tileMin.x, tileMin.y), 0.0);  // bottom-left
    corners[1] = uv_to_view(vec2(tileMax.x, tileMin.y), 0.0);  // bottom-right
    corners[2] = uv_to_view(vec2(tileMax.x, tileMax.y), 0.0);  // top-right
    corners[3] = uv_to_view(vec2(tileMin.x, tileMax.y), 0.0);  // top-left

	vec3 normals[4];
    normals[0] = compute_plane_normal(corners[1], corners[0]);  // bottom
    normals[1] = compute_plane_normal(corners[3], corners[2]);  // top
    normals[2] = compute_plane_normal(corners[0], corners[3]);  // left
    normals[3] = compute_plane_normal(corners[2], corners[1]);  // right

	/* --- Pass 1: Count visible lights --- */
	const mat3 viewRot = mat3(uView);
	for (uint i = flatThread; i < uPointLightCount; i += GROUP_SIZE)
	{
		const PointLight light = uPointLights[i];
		vec3 posVS = (uView * vec4(light.position.xyz, 1.0)).xyz;
		float radius = light.color.w;

		if (posVS.z - radius > minZ) continue;
		if (posVS.z + radius < maxZ) continue;
		if (sphere_intersects_frustum(posVS, radius, normals))
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
		if (spot_intersects_frustum(posVS, dirVS, range, outerCutoff, normals))
			atomicAdd(sSpotLightCount, 1u);
	}
	barrier();

	if (flatThread == 0u)
	{
		sBaseOffset = atomicAdd(oGlobalLightCount,
			sPointLightCount + sSpotLightCount);
        sPointLightCount = 0u;
        sSpotLightCount  = 0u;
	}
	barrier();

	/* --- Pass 2: Write indices into the reserved block --- */
	for (uint i = flatThread; i < uPointLightCount; i += GROUP_SIZE)
	{
		const PointLight light = uPointLights[i];
		vec3 posVS = (uView * vec4(light.position.xyz, 1.0)).xyz;
		float radius = light.color.w;

		if (posVS.z - radius > minZ) continue;
		if (posVS.z + radius < maxZ) continue;
		if (sphere_intersects_frustum(posVS, radius, normals))
            oLightIndexList[sBaseOffset + atomicAdd(sPointLightCount, 1u)] = i;
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
		if (spot_intersects_frustum(posVS, dirVS, range, outerCutoff, normals))
            oLightIndexList[sBaseOffset + sPointLightCount +
				atomicAdd(sSpotLightCount, 1u)] = i;
	}
	barrier();

	/* --- Write light grid entry for this tile --- */
	if (flatThread == 0u)
	{
		oLightGrid[tileID.y * uTileCountX + tileID.x] = uvec2(sBaseOffset,
            (sPointLightCount << 16) | sSpotLightCount);
	}
}