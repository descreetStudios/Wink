#version 460 core

#extension GL_ARB_bindless_texture : require

#include "ToneMapping.glsl"
#include "Material.glsl"
#include "PBR.glsl"
#include "Light.glsl"
#include "Pipeline/PoissonDisk.glsl"

// #define DEBUG_ALBEDO
// #define DEBUG_NORMALS
// #define DEBUG_ROUGHNESS
// #define DEBUG_METALLIC
// #define DEBUG_AO
// #define DEBUG_EMISSIVE
// #define DEBUG_IN

#include "Debug.glsl"

#define IBL_MAX_MIPS 5.0
#define NUM_CASCADES 4

in vec3 vDebug;
in vec3 vFragPos;
in vec3 vCamPos;
in mat4 vView;
in vec2 vTexCoord;
in vec2 vTexCoord1;
in mat3 vTBN;

flat in uint vTileCountX;

out vec4 FragColor;

uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilteredMap;
uniform sampler2D uBRDFLUT;
uniform bool uHasIBL;

layout(binding = 15) uniform sampler2DArrayShadow uShadowMap;
layout(binding = 16) uniform sampler2DArray uShadowMapRaw;

layout(std140, binding = 1) uniform LightsUBO
{
	uint uDirLightCount;
	uint _lp0; uint _lp1; uint _lp2;

	DirLight uDirLights[MAX_DIR_LIGHTS];
};

layout(std140, binding = 4) uniform ShadowUBO
{
	mat4 uLightSpaceMatrices[NUM_CASCADES];
	vec4 uCascadeSplits;
	vec4 uCascadeOrthoSizes;
	float uShadowMapTexelSize;
	float _sp0[3];
};

layout(std430, binding = 0) readonly buffer PointLightBuffer
{
	PointLight uPointLights[];
};

layout(std430, binding = 1) readonly buffer SpotLightBuffer
{
	SpotLight uSpotLights[];
};

layout(std430, binding = 5) readonly buffer LightIndexListBuffer
{
	uint oLightIndexList[];
};

layout(std430, binding = 6) readonly buffer LightGridBuffer
{
	uvec2 oLightGrid[]; // .x = base offset, .y = (pointCount << 16) | spotCount
};

/* --- Helpers --- */
vec2 get_uv_loc(int loc)
{
	return loc == 1 ? vTexCoord1 : vTexCoord;
}

uint get_tile_index()
{
	uvec2 tileID = uvec2(gl_FragCoord.xy) / uvec2(TILE_SIZE);
	return tileID.y * vTileCountX + tileID.x;
}

/* --- Shadows --- */
const float BLOCKER_SAMPLES = 32.0;
const float PCF_SAMPLES = 32.0;

const float SHADOW_BIAS_MIN = 0.00001;
const float DEPTH_SLOPE_BIAS = 0.2;
const float RECEIVER_PLANE_BIAS = 0.2;

const float NORMAL_OFFSET_SCALE = 0.5;

const float PCF_RADIUS = 1.0;

const float SHADOW_NOISE_STRENGTH = 1.0;

int select_cascade(float depthVS)
{
	for (int i = 0; i < NUM_CASCADES - 1; ++i)
		if (depthVS < uCascadeSplits[i]) return i;
	return NUM_CASCADES - 1;
}

float ign(vec2 fragCoord)
{
	return fract(52.9829189 * fract(dot(
		fragCoord, vec2(0.06711056, 0.00583715))));
}

vec2 rotate_poisson(vec2 sam, float theta)
{
	float s = sin(theta), c = cos(theta);
	return vec2(c * sam.x - s * sam.y,
		s * sam.x + c * sam.y);
}

float find_avg_blocker_depth(
	int cascade, vec2 uv, float zReceiver,
	float bias, float theta)
{
	float blockerRadius = uCascadeOrthoSizes[cascade];

	float total = 0.0;
	int count = 0;

	for (int i = 0; i < BLOCKER_SAMPLES; ++i)
	{
		vec2 s = rotate_poisson(POISSON_DISK[i], theta);
		float zBlocker = texture(uShadowMapRaw,
			vec3(uv + s * blockerRadius, float(cascade))).r;

		if (zBlocker < zReceiver - bias)
		{
			total += zBlocker;
			++count;
		}
	}

	if (count == 0 || count == int(BLOCKER_SAMPLES)) return -2.0;
	return total / float(count);
}

float pcss_pcf(int cascade, vec2 uv, float zReceiver,
	float bias, float penumbraUV, float theta)
{
	float pcfRadius = penumbraUV * PCF_RADIUS;

	float shadow = 0.0;
	for (int i = 0; i < PCF_SAMPLES; ++i)
	{
		vec2 s = rotate_poisson(POISSON_DISK[i], theta);
		shadow += texture(uShadowMap,
			vec4(uv + s * pcfRadius, float(cascade), zReceiver - bias));
	}
	return shadow / float(PCF_SAMPLES);
}

float compute_shadow(vec3 fragPosWS, vec3 N, vec3 L, float depthVS)
{
	int cascade = select_cascade(depthVS);

	float texelWorldSize = uCascadeOrthoSizes[cascade] * uShadowMapTexelSize;
	vec3 normalOffset = N * texelWorldSize * 
		NORMAL_OFFSET_SCALE * (1.0 - max(dot(N, L), 0.0));

	vec4 offsetPosLS = uLightSpaceMatrices[cascade] * vec4(fragPosWS + normalOffset, 1.0);
	vec3 offsetCoords = offsetPosLS.xyz / offsetPosLS.w * 0.5 + 0.5;

	vec2 uv = offsetCoords.xy;
	float zReceiver = offsetCoords.z;

	if (zReceiver > 1.0
		|| any(lessThan(uv, vec2(0.0)))
		|| any(greaterThan(uv, vec2(1.0))))
		return 0.0;

	float ndl = max(dot(N, L), 0.0);
	float slopeBias = DEPTH_SLOPE_BIAS * (1.0 - ndl) * uShadowMapTexelSize;
	float rpBias = RECEIVER_PLANE_BIAS * uShadowMapTexelSize;
	float bias = SHADOW_BIAS_MIN + slopeBias + rpBias;

	float theta = ign(gl_FragCoord.xy) * 6.28318530718 * SHADOW_NOISE_STRENGTH;

	float avgBlocker = find_avg_blocker_depth(cascade, uv, zReceiver, bias, theta);

	if (avgBlocker == -2.0)
		return 1.0 - pcss_pcf(cascade, uv, zReceiver, bias,
			uShadowMapTexelSize * 2.0, theta);
	if (avgBlocker != avgBlocker) return 0.0;

	float lightSizeUV = 1.0 / uCascadeOrthoSizes[cascade];
	float penumbraUV = ((zReceiver - avgBlocker) / avgBlocker) * lightSizeUV;
	penumbraUV = clamp(penumbraUV,
		uCascadeOrthoSizes[cascade],
		uCascadeOrthoSizes[cascade]);

	float shadow = 1.0 - pcss_pcf(cascade, uv, zReceiver, bias, penumbraUV, theta);

	if (cascade < NUM_CASCADES - 1)
	{
		float splitDepth = uCascadeSplits[cascade];
		float blendRange = splitDepth;
		float blendFactor = smoothstep(splitDepth - blendRange, splitDepth, depthVS);

		if (blendFactor > 0.0)
		{
			int nextCascade = cascade + 1;
			float nextTexelWorld = uCascadeOrthoSizes[nextCascade] * uShadowMapTexelSize;
			vec3 nextNormalOffset = N * nextTexelWorld * 1.5 * (1.0 - max(dot(N, L), 0.0));

			vec4 nextPosLS = uLightSpaceMatrices[nextCascade]
				* vec4(fragPosWS + nextNormalOffset, 1.0);
			vec3 nextCoords = nextPosLS.xyz / nextPosLS.w * 0.5 + 0.5;
			vec2 nextUV = nextCoords.xy;
			float nextZ = nextCoords.z;

			float nextBlocker = find_avg_blocker_depth(nextCascade,
				nextUV, nextZ, bias, theta);

			float shadowNext;
			if (nextBlocker == -2.0)
				shadowNext = 1.0 - pcss_pcf(nextCascade, nextUV, nextZ, bias,
					uShadowMapTexelSize * 2.0, theta);
			else if (nextBlocker != nextBlocker) shadowNext = 0.0;
			else
			{
				float nextLightSizeUV = 1.0 / uCascadeOrthoSizes[nextCascade];
				float nextPenumbra = ((nextZ - nextBlocker) / nextBlocker) * nextLightSizeUV;
				nextPenumbra = clamp(nextPenumbra,
					uShadowMapTexelSize * 2.0,
					uShadowMapTexelSize * 8.0);
				shadowNext = 1.0 - pcss_pcf(nextCascade, nextUV, nextZ,
					bias, nextPenumbra, theta);
			}

			shadow = mix(shadow, shadowNext, blendFactor);
		}
	}

	return shadow;
}

/* --- Light Functions --- */
vec3 compute_dir_light(DirLight light,
	vec3 albedo, vec3 N, vec3 V,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 L = normalize(-light.direction.xyz);
	float NdL = dot(N, L);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float D = distribution_ggx(N, H, roughness);
	float G = geometry_smith(N, V, L, roughness);
	vec3 F = fresnel_schlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 radiance = light.color.rgb * light.direction.w; // .w = intensity

	float depthVS = abs((vView * vec4(vFragPos, 1.0)).z);
	float shadow = compute_shadow(vFragPos, N, L, depthVS);

	return (kD * albedo / PI + specular) * radiance * NdL * (1.0 - shadow);
}

vec3 compute_point_light(PointLight light,
	vec3 albedo, vec3 N, vec3 V, vec3 fragPos,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 toLight = light.position.xyz - fragPos;
	float dist = length(toLight);
	float radius = light.color.w; // .w = radius

	float window = 1.0 - smoothstep(radius * 0.75, radius, dist);
	if (window <= 0.0) return vec3(0.0);

	vec3 L = toLight / dist;
	float NdL = dot(N, L);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float D = distribution_ggx(N, H, roughness);
	float G = geometry_smith(N, V, L, roughness);
	vec3 F = fresnel_schlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	float attenuation = (light.position.w * window) / (dist * dist + 1.0); // .w = intensity
	vec3 radiance = light.color.rgb * attenuation;

	return (kD * albedo / PI + specular) * radiance * NdL;
}

vec3 compute_spot_light(SpotLight light,
	vec3 albedo, vec3 N, vec3 V, vec3 fragPos,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 toLight = light.position.xyz - fragPos;
	float dist = length(toLight);
	float range = light.position.w; // .w = range

	float window = 1.0 - smoothstep(range * 0.75, range, dist);
	if (window <= 0.0) return vec3(0.0);

	vec3 L = toLight / dist;
	float theta = dot(L, normalize(-light.direction.xyz));
	float outerCutoff  = light.color.w; // .w = outerCutoff
	float epsilon = light.inner.x - outerCutoff;
	float spotFactor = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
	if (spotFactor <= 0.0) return vec3(0.0);

	float NdL = dot(N, L);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float D = distribution_ggx(N, H, roughness);
	float G = geometry_smith(N, V, L, roughness);
	vec3 F = fresnel_schlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	float attenuation = (light.direction.w * window * spotFactor) /
		(dist * dist + 1.0);  // .w = intensity
	vec3 radiance = light.color.rgb * attenuation;

	return (kD * albedo / PI + specular) * radiance * NdL;
}

/* --- IBL Ambient --- */
vec3 compute_ibl(vec3 albedo, vec3 N, vec3 V,
	float NdV, float metallic, float roughness, float ao, vec3 F0)
{
	const float IBL_FRESNEL_SCALE = 0.85;
	const float IBL_IRRADIANCE_SCALE = 0.25;
	const float IBL_SPECULAR_SCALE = 1.25;

	vec3 F = fresnel_schlick_roughness(NdV, F0, roughness) * IBL_FRESNEL_SCALE;
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 irradiance = texture(uIrradianceMap, N).rgb * IBL_IRRADIANCE_SCALE;
	vec3 diffuse = kD * irradiance * albedo;

	vec3 R = reflect(-V, N);
	vec3 prefilteredColor = textureLod(
		uPrefilteredMap, R, roughness * IBL_MAX_MIPS).rgb * IBL_SPECULAR_SCALE;
	vec2 brdf = texture(uBRDFLUT, vec2(NdV, roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	return (diffuse + specular) * ao;
}

/* --- PBR Shading --- */
vec3 compute_pbr(vec3 albedo, vec3 N, vec3 V,
	float metallic, float roughness, float ao, vec3 emissive)
{
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	float NdV = max(dot(N, V), 0.0);

	vec3 Lo = vec3(0.0);

	for (int i = 0; i < uDirLightCount; ++i)
		Lo += compute_dir_light(uDirLights[i],
			albedo, N, V, NdV, metallic, roughness, F0);

	uint tileIndex = get_tile_index();
	uint pointOffset = oLightGrid[tileIndex].x;
	uint pointCount = oLightGrid[tileIndex].y >> 16;
	uint spotCount = oLightGrid[tileIndex].y & 0xFFFFu;

	for (uint i = 0u; i < pointCount; ++i)
	{
		uint idx = oLightIndexList[pointOffset + i];
		Lo += compute_point_light(uPointLights[idx],
			albedo, N, V, vFragPos, NdV, metallic, roughness, F0);
	}

	for (uint i = 0u; i < spotCount; ++i)
	{
		uint idx = oLightIndexList[pointOffset + pointCount + i];
		Lo += compute_spot_light(uSpotLights[idx],
			albedo, N, V, vFragPos, NdV, metallic, roughness, F0);
	}

	vec3 ambient = uHasIBL
		? compute_ibl(albedo, N, V, NdV, metallic, roughness, ao, F0)
		: albedo * F0 * (0.2 * ao);

	return Lo + ambient + emissive;
}

void main()
{
	vec4 albedo = uMaterial.baseColor * 
		SAMPLE_BINDLESS(uAlbedoHandle, get_uv_loc(uMaterial.albedoTexCoord));

	vec3 tsN = SAMPLE_BINDLESS(uNormalHandle,
		get_uv_loc(uMaterial.normalTexCoord)).rgb * 2.0 - 1.0;
	vec3 N = normalize(vTBN * tsN);

	vec2 mr = SAMPLE_BINDLESS(uMRHandle, get_uv_loc(uMaterial.mrTexCoord)).gb;
	float roughness = uMaterial.roughness * mr.x;
	float metallic = uMaterial.metallic  * mr.y;

	float ao = mix(1.0, SAMPLE_BINDLESS(uAOHandle,
		get_uv_loc(uMaterial.aoTexCoord)).r, uMaterial.aoStrength);

	vec3 emissive = uMaterial.emissiveFactor.rgb *
		SAMPLE_BINDLESS(uEmissiveHandle, get_uv_loc(uMaterial.emissiveTexCoord)).rgb;

	vec4 debugColor;
	if (apply_debug_channels(debugColor, albedo, N, roughness, metallic, ao, emissive))
	{
		FragColor = debugColor;
		return;
	}

	vec3 V = normalize(vCamPos - vFragPos);
	vec3 color = compute_pbr(albedo.rgb, N, V, metallic, roughness, ao, emissive);

#if 0
	float depthVS = abs((vView * vec4(vFragPos, 1.0)).z);
	int cascade = select_cascade(depthVS);
	if (cascade == 0) { FragColor = vec4(1,0,0,1); return; }
	if (cascade == 1) { FragColor = vec4(0,1,0,1); return; }
	if (cascade == 2) { FragColor = vec4(0,0,1,1); return; }
	else { FragColor = vec4(1,1,0,1); return; }
#endif

	// TODO: move into post-process pass
	color = tonemap_lottes(color, 2.0);
	color = apply_gamma(color, 2.2);

#if defined(DEBUG_IN)
	FragColor = vec4(vDebug, 1.0);
#else
	FragColor = vec4(color, albedo.a);
#endif
}
