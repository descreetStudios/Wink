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

in vec3 vDebug;
in vec3 vFragPos;
in vec3 vCamPos;
in vec2 vTexCoord;
in vec2 vTexCoord1;
in mat3 vTBN;

flat in uint vTileCountX;

out vec4 FragColor;

uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilteredMap;
uniform sampler2D uBRDFLUT;
uniform bool uHasIBL;

#define SHADOW_MAP_SIZE 4096
uniform float uTime;
uniform sampler2DShadow uShadowMap;
uniform sampler2D uShadowMapRaw;
uniform mat4 uLightSpaceMatrix;
uniform float uLightOrthoSize;

layout(std140, binding = 1) uniform LightsUBO
{
	uint uDirLightCount;
	uint _lp0;
	uint _lp1;
	uint _lp2;

	DirLight uDirLights[MAX_DIR_LIGHTS];
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

/* --- Shadow --- */
const float LIGHT_WORLD_SIZE = 1.0;
float LIGHT_SIZE_UV = LIGHT_WORLD_SIZE / uLightOrthoSize;
float BLOCKER_RADIUS = LIGHT_SIZE_UV * 4.0;

const float BLOCKER_SAMPLES = 24.0;
const float PCF_SAMPLES = 64.0;

float ign(vec2 fragCoord)
{
    return fract(52.9829189 * fract(dot(fragCoord, vec2(0.06711056, 0.00583715))));
}

vec2 rotate_poisson(vec2 sam, float theta)
{
    float s = sin(theta), c = cos(theta);
    return vec2(c * sam.x - s * sam.y,
		s * sam.x + c * sam.y);
}

float find_avg_blocker_depth(vec2 uv, float z_receiver, float bias, float theta)
{
    float total = 0.0;
    int count = 0;

	if (BLOCKER_SAMPLES > MAX_POISSON_SAMPLES) return 0.0;
    for (int i = 0; i < int(BLOCKER_SAMPLES); ++i)
    {
        vec2 s = rotate_poisson(POISSON_DISK[i], theta);
        vec2 sample_uv = uv + s * BLOCKER_RADIUS;
        float z_blocker = texture(uShadowMapRaw, sample_uv).r;

        if (z_blocker < z_receiver - bias)
        {
            total += z_blocker;
            ++count;
        }
    }

    //if (count == 0) return -1.0;
    if (count == int(BLOCKER_SAMPLES)) return -2.0;
    return total / float(count);
}

float pcss_pcf(vec2 uv, float z_receiver, float bias, float penumbra_uv, float theta)
{
    float shadow = 0.0;

	if (PCF_SAMPLES > MAX_POISSON_SAMPLES) return shadow / MAX_POISSON_SAMPLES;
    for (int i = 0; i < PCF_SAMPLES; ++i)
    {
        vec2 s = rotate_poisson(POISSON_DISK[i], theta);
        shadow += texture(uShadowMap,
            vec3(uv + s * penumbra_uv, z_receiver - bias));
    }
    return shadow / float(PCF_SAMPLES);
}

float compute_shadow(vec3 fragPosWS, vec3 N, vec3 L)
{
    vec4 fragPosLS = uLightSpaceMatrix * vec4(fragPosWS, 1.0);
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0
        || any(lessThan(projCoords.xy, vec2(0.0)))
        || any(greaterThan(projCoords.xy, vec2(1.0))))
        return 0.0;

	vec2 uv = projCoords.xy;
    float z_receiver = projCoords.z;

	float texelWorldSize = uLightOrthoSize / float(SHADOW_MAP_SIZE);
    vec3 normalOffset = N * texelWorldSize * 1.5 * (1.0 - max(dot(N, L), 0.0));
    vec4 offsetPosLS = uLightSpaceMatrix * vec4(fragPosWS + normalOffset, 1.0);
    vec3 offsetCoords = offsetPosLS.xyz / offsetPosLS.w * 0.5 + 0.5;

	uv = offsetCoords.xy;
    z_receiver = offsetCoords.z;

	float bias = 0.0001;

	float theta = ign(gl_FragCoord.xy + fract(uTime) * 100.0) * 6.28318530718;
	//float theta = ign(gl_FragCoord.xy) * 6.28318530718;
    float avg_blocker = find_avg_blocker_depth(uv, z_receiver, bias, theta);

    if (avg_blocker == -1.0) return 0.0;
	if (avg_blocker == -2.0)
	{
		float texel = 1.0 / float(textureSize(uShadowMap, 0).x);
		return 1.0 - pcss_pcf(uv, z_receiver, bias, texel * 2.0, theta);
	}

    // Penumbra estimation
    float penumbra_uv = ((z_receiver - avg_blocker) / avg_blocker) * LIGHT_SIZE_UV;
    penumbra_uv = max(penumbra_uv, 1.0 / textureSize(uShadowMap, 0).x);

    // PCSS stage 2: Variable-Width PCF
    return 1.0 - pcss_pcf(uv, z_receiver, bias, penumbra_uv, theta);
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

	float shadow = compute_shadow(vFragPos, N, L);

	//return vec3(shadow);
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

	// TODO: move into post-process pass
	color = tonemap_lottes(color, 2.0);
	color = apply_gamma(color, 2.2);

	//vec3 L = normalize(-uDirLights[0].direction.xyz);
	//float shadow = compute_shadow(vFragPos, N, L);
    //FragColor = vec4(vec3(shadow), 1.0);
    //return;

#if defined(DEBUG_IN)
	FragColor = vec4(vDebug, 1.0);
#else
	FragColor = vec4(color, albedo.a);
#endif
}
