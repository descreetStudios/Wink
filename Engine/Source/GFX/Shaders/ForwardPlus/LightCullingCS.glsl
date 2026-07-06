#version 460 core

#define MAX_LIGHTS_PER_TILE 256

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

uniform mat4 uView;
uniform mat4 uInvProj;
uniform uint uScreenWidth;
uniform uint uScreenHeight;
uniform uint uTileCountX;
uniform uint uPointLightCount;
uniform uint uSpotLightCount;
uniform sampler2D uDepth;

struct PointLight
{
	vec4 position;   // .xyz = position,  .w = intensity
	vec4 color;      // .xyz = color,     .w = radius
};

struct SpotLight
{
	vec4 position;   // .xyz = position,  .w = range
	vec4 direction;  // .xyz = direction, .w = intensity
	vec4 color;      // .xyz = color,     .w = outerCutoff
	vec4 inner;      // .x  = innerCutoff
};

layout(std430, binding = 0) readonly buffer PointLightBuffer
{
	PointLight uPointLights[];
};

layout(std430, binding = 1) readonly buffer SpotLightBuffer
{
	SpotLight uSpotLights[];
};

layout(std430, binding = 2) writeonly buffer LightIndexList
{
	uint oLightIndexList[];
};

layout(std430, binding = 3) buffer LightGrid
{
	uvec2 oLightGrid[]; // .x = offset (derived), .y = count
};

shared uint sMinDepthInt;
shared uint sMaxDepthInt;
shared uint sPointLightCount;
shared uint sSpotLightCount;
shared uint sPointLightIndices[MAX_LIGHTS_PER_TILE];
shared uint sSpotLightIndices[MAX_LIGHTS_PER_TILE];

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

bool sphere_inside_plane(vec4 plane, vec3 center, float radius)
{
	return dot(plane.xyz, center) + plane.w >= -radius;
}

bool sphere_intersects_frustum(
	vec3 center, float radius, vec4 planes[6])
{
	for (int i = 0; i < 6; ++i)
		if (!sphere_inside_plane(planes[i], center, radius))
			return false;
	return true;
}

bool spot_intersects_frustum(vec3 posVS, vec3 dirVS,
    float range, float outerCutoff, vec4 planes[6])
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

float linearize(float depth)
{
    float ndcZ = depth * 2.0 - 1.0;
    vec4 view = uInvProj * vec4(0.0, 0.0, ndcZ, 1.0);
    return view.z / view.w;
}

void main()
{
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	ivec2 threadID = ivec2(gl_LocalInvocationID.xy);
	uint flatThread = gl_LocalInvocationIndex;

	if (flatThread == 0)
	{
		sMinDepthInt = 0xFFFFFFFF;
		sMaxDepthInt = 0;
		sPointLightCount = 0;
		sSpotLightCount  = 0;
	}
	barrier();

	ivec2 pixelCoord = tileID * 16 + threadID;
	pixelCoord = clamp(pixelCoord,
		ivec2(0), ivec2(uScreenWidth - 1, uScreenHeight - 1));

	vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(uScreenWidth, uScreenHeight);
	float depth = texture(uDepth, uv).r;

	uint depthInt = floatBitsToUint(depth);
	atomicMin(sMinDepthInt, depthInt);
	atomicMax(sMaxDepthInt, depthInt);
	barrier();

	float minDepth = uintBitsToFloat(sMinDepthInt);
	float maxDepth = uintBitsToFloat(sMaxDepthInt);

	vec2 tileMin = vec2(tileID) / vec2(uTileCountX, gl_NumWorkGroups.y);
	vec2 tileMax = vec2(tileID + 1) / vec2(uTileCountX, gl_NumWorkGroups.y);

	vec3 corners[4];
	corners[0] = uv_to_view(vec2(tileMin.x, tileMin.y), 0.5);
	corners[1] = uv_to_view(vec2(tileMax.x, tileMin.y), 0.5);
	corners[2] = uv_to_view(vec2(tileMax.x, tileMax.y), 0.5);
	corners[3] = uv_to_view(vec2(tileMin.x, tileMax.y), 0.5);

	vec3 origin = vec3(0.0);

	vec4 planes[6];
	planes[0] = compute_plane(origin, corners[0], corners[1]); // bottom
	planes[1] = compute_plane(origin, corners[2], corners[3]); // top
	planes[2] = compute_plane(origin, corners[3], corners[0]); // left
	planes[3] = compute_plane(origin, corners[1], corners[2]); // right

	vec2 tileCenter = (vec2(tileID) + 0.5) / vec2(uTileCountX, gl_NumWorkGroups.y);

	float minZ = linearize(minDepth);
	float maxZ = linearize(maxDepth);

	vec4 cameraNearPlane = vec4(0.0, 0.0, -1.0, 0.0);
	float nearClipVS = uv_to_view(vec2(0.5), 0.0).z;

	planes[4] = vec4(0.0, 0.0, -1.0,  nearClipVS);
	planes[5] = vec4(0.0, 0.0,  1.0, -minZ);

	/* --- Point Light Culling --- */
	uint groupSize = gl_WorkGroupSize.x * gl_WorkGroupSize.y;

	for (uint i = flatThread; i < uPointLightCount; i += groupSize)
	{
		vec3 posVS = (uView * vec4(uPointLights[i].position.xyz, 1.0)).xyz;
		float radius = uPointLights[i].color.w; // .w = radius

		if (sphere_intersects_frustum(posVS, radius, planes))
		{
			uint slot = atomicAdd(sPointLightCount, 1);
			if (slot < MAX_LIGHTS_PER_TILE) sPointLightIndices[slot] = i;
		}
	}

	/* --- Spot Light Culling --- */
	for (uint i = flatThread; i < uSpotLightCount; i += groupSize)
	{
		vec3 posVS = (uView * vec4(uSpotLights[i].position.xyz, 1.0)).xyz;
		vec3 dirVS = normalize(mat3(uView) * uSpotLights[i].direction.xyz);
		float range = uSpotLights[i].position.w; // .w = range
		float outerCutoff = uSpotLights[i].color.w; // .w = outerCutoff

		if (spot_intersects_frustum(posVS, dirVS, range, outerCutoff, planes))
		{
			uint slot = atomicAdd(sSpotLightCount, 1);
			if (slot < MAX_LIGHTS_PER_TILE) sSpotLightIndices[slot] = i;
		}
	}
	barrier();

	/* --- Write results to SSBOs --- */
	if (flatThread == 0)
	{
		uint tileIndex  = tileID.y * uTileCountX + tileID.x;
		uint baseOffset = tileIndex * MAX_LIGHTS_PER_TILE;

		// Write point lights
		uint pointCount = min(sPointLightCount, MAX_LIGHTS_PER_TILE);
		for (uint i = 0; i < pointCount; ++i)
			oLightIndexList[baseOffset + i] = sPointLightIndices[i];

		uint spotCount = min(sSpotLightCount, MAX_LIGHTS_PER_TILE - pointCount);
		for (uint i = 0; i < spotCount; ++i)
			oLightIndexList[baseOffset + pointCount + i] = sSpotLightIndices[i];

		oLightGrid[tileIndex] = uvec2(pointCount, spotCount);
	}
}