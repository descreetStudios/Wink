#version 460 core

#extension GL_ARB_bindless_texture : require

#include "ToneMapping.glsl"
#include "Material.glsl"
#include "PBR.glsl"
#include "Light.glsl"

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

uniform sampler2DShadow uShadowMap;
uniform mat4 uLightSpaceMatrix;

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
// Poisson disk samples for soft shadows
const vec2 POISSON_DISK[16] = vec2[](
    vec2(-0.94201624,  -0.39906216),
    vec2( 0.94558609,  -0.76890725),
    vec2(-0.09418410,  -0.92938870),
    vec2( 0.34495938,   0.29387760),
    vec2(-0.91588581,   0.45771432),
    vec2(-0.81544232,  -0.87912464),
    vec2(-0.38277543,   0.27676845),
    vec2( 0.97484398,   0.75648379),
    vec2( 0.44323325,  -0.97511554),
    vec2( 0.53742981,  -0.47373420),
    vec2(-0.26496911,  -0.41893023),
    vec2( 0.79197514,   0.19090188),
    vec2(-0.24188840,   0.99706507),
    vec2(-0.81409955,   0.91437590),
    vec2( 0.19984126,   0.78641367),
    vec2( 0.14383161,  -0.14100790)
);

float compute_shadow(vec3 fragPosWS, vec3 N, vec3 L)
{
    vec4 fragPosLS = uLightSpaceMatrix * vec4(fragPosWS, 1.0);
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0
        || any(lessThan(projCoords.xy, vec2(0.0)))
        || any(greaterThan(projCoords.xy, vec2(1.0))))
        return 0.0;

    float cosTheta = clamp(dot(N, L), 0.0, 1.0);
    float bias = clamp(0.005 * tan(acos(cosTheta)), 0.0, 0.0003);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float spread = 2.0;

    for (int i = 0; i < 16; ++i)
    {
        shadow += texture(uShadowMap,
            vec3(projCoords.xy + POISSON_DISK[i] * texelSize * spread,
                 projCoords.z - bias));
    }

    return 1.0 - (shadow / 16.0);
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
