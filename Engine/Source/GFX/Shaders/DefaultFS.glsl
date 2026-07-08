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

in vec3 vCamPos;
in vec3 vFragPos;
in vec2 vTexCoord;
in vec2 vTexCoord1;
in mat3 vTBN;

flat in uint vTileCountX;
flat in uint vScreenWidth;
flat in uint vScreenHeight;

in vec3 vDebug;

out vec4 FragColor;

uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilteredMap;
uniform sampler2D uBRDFLUT;
uniform bool uHasIBL;

#define TILE_SIZE 16

layout(std430, binding = 0) readonly buffer PointLightBuffer
{
    PointLight uPointLights[];
};

layout(std430, binding = 1) readonly buffer SpotLightBuffer
{
    SpotLight uSpotLights[];
};

layout(std430, binding = 3) readonly buffer TileLightIndexList
{
	uint oLightIndexList[];
};

layout(std430, binding = 4) readonly buffer TileLightGrid
{
	uvec2 oLightGrid[]; // .x = pointCount, .y = spotCount
};

uint get_tile_index()
{
	uvec2 tileID = uvec2(gl_FragCoord.xy) / uvec2(TILE_SIZE);
	return tileID.y * vTileCountX + tileID.x;
}

/* --- TexCoord Location Helper --- */
vec2 get_uv_loc(int loc)
{
	return loc == 1 ? vTexCoord1 : vTexCoord;
}

/* --- Directional Light --- */
vec3 compute_dir_light(DirLight light,
	vec3 albedo, vec3 N, vec3 V,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 L = normalize(-light.direction.xyz);
	float NdL = max(dot(N, L), 0.0);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float D = distribution_ggx(N, H, roughness);
	float G = geometry_smith(N, V, L, roughness);
	vec3 F = fresnel_schlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse = kD * albedo / PI;

	vec3 radiance = light.color.rgb * light.direction.w; // .w = intensity
	return (diffuse + specular) * radiance * NdL;
}

/* --- Point Light --- */
vec3 compute_point_light(PointLight light,
	vec3 albedo, vec3 N, vec3 V, vec3 fragPos,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 toLight = light.position.xyz - fragPos;
	float dist = length(toLight);

	float window = 1.0 - smoothstep(light.color.w * 0.75, light.color.w, dist); // .w = radius
	if (window <= 0.0) return vec3(0.0);

	vec3 L = toLight / dist;
	float NdL = max(dot(N, L), 0.0);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float attenuation = (light.position.w * window) / (dist * dist + 1.0); // .w = intensity

	float D = distribution_ggx(N, H, roughness);
	float G = geometry_smith(N, V, L, roughness);
	vec3 F = fresnel_schlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse = kD * albedo / PI;

	vec3 radiance = light.color.rgb * attenuation;
	return (diffuse + specular) * radiance * NdL;
}

/* --- Spot Light --- */
vec3 compute_spot_light(SpotLight light,
	vec3 albedo, vec3 N, vec3 V, vec3 fragPos,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 toLight = light.position.xyz - fragPos;
	float dist = length(toLight);

	float window = 1.0 - smoothstep(light.position.w * 0.75, light.position.w, dist); // .w = range
	if (window <= 0.0) return vec3(0.0);

	vec3 L = toLight / dist;
	float theta = dot(L, normalize(-light.direction.xyz));

	float epsilon = light.inner.x - light.color.w; // .w = outerCutoff
	float spotIntensity = clamp((theta - light.color.w) / epsilon, 0.0, 1.0); // .w = outerCutoff
	if (spotIntensity <= 0.0) return vec3(0.0);

	float NdL = max(dot(N, L), 0.0);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float D = distribution_ggx(N, H, roughness);
	float G = geometry_smith(N, V, L, roughness);
	vec3 F = fresnel_schlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse = kD * albedo / PI;

	float attenuation = (light.direction.w * window * spotIntensity) / (dist * dist + 1.0); // .w = intensity
	vec3 radiance = light.color.rgb * attenuation;

	return (diffuse + specular) * radiance * NdL;
}

/* --- IBL Ambient --- */
vec3 compute_ibl(vec3 albedo, vec3 N, vec3 V,
	float NdV, float metallic, float roughness,
	float ao, vec3 F0)
{
	vec3 F = fresnel_schlick_roughness(NdV, F0, roughness) * 0.85;
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 irradiance = texture(uIrradianceMap, N).rgb * 0.25;
	vec3 diffuse = kD * irradiance * albedo;

	vec3 R = reflect(-V, N);
	float MAX_MIPS = 5.0;
	vec3 prefilteredColor = textureLod(uPrefilteredMap, R, roughness * MAX_MIPS).rgb * 1.25;
	vec2 brdf = texture(uBRDFLUT, vec2(NdV, roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	return (diffuse + specular) * ao;
}

/* --- PBR Shading --- */
vec3 compute_pbr(vec3 albedo, vec3 N, vec3 V,
	float metallic, float roughness,
	float ao, vec3 emissive)
{
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	float NdV = max(dot(N, V), 0.0);

	vec3 directLight = vec3(0.0);

	for (int i = 0; i < uDirLightCount; ++i)
		directLight += compute_dir_light(uDirLights[i],
			albedo, N, V, NdV, metallic, roughness, F0);

	uint tileIndex = get_tile_index();
	uint pointOffset = oLightGrid[tileIndex].x;
	uint pointCount = oLightGrid[tileIndex].y >> 16;
	uint spotCount = oLightGrid[tileIndex].y & 0xFFFF;

	for (uint i = 0; i < pointCount; ++i)
    {
        uint idx = oLightIndexList[pointOffset + i];
        directLight += compute_point_light(uPointLights[idx],
            albedo, N, V, vFragPos, NdV, metallic, roughness, F0);
    }

    for (uint i = 0; i < spotCount; ++i)
    {
        uint idx = oLightIndexList[pointOffset + pointCount + i];
        directLight += compute_spot_light(uSpotLights[idx],
            albedo, N, V, vFragPos, NdV, metallic, roughness, F0);
    }

	vec3 ambient;
	uHasIBL ? ambient = compute_ibl(albedo, N, V,
			NdV, metallic, roughness, ao, F0) :
		ambient = albedo * F0 * (0.2 * ao);

	return directLight + ambient + emissive;
}

void main()
{

	/* --- Albedo --- */
	vec2 uv = get_uv_loc(uMaterial.albedoTexCoord);
	vec4 albedo = uMaterial.baseColor *
		SAMPLE_BINDLESS(uAlbedoHandle, uv);

	/* --- Normals --- */
	uv = get_uv_loc(uMaterial.normalTexCoord);
	vec3 tsN = SAMPLE_BINDLESS(uNormalHandle, uv).rgb * 2.0 - 1.0;
	vec3 N = normalize(vTBN * tsN);

	/* --- Metallic & Roughness --- */
	uv = get_uv_loc(uMaterial.mrTexCoord);
	vec2 mr = SAMPLE_BINDLESS(uMRHandle, uv).gb;
	float roughness = uMaterial.roughness * mr.x;
	float metallic = uMaterial.metallic * mr.y;

	/* --- Ambient Occlusion --- */
	uv = get_uv_loc(uMaterial.aoTexCoord);
	float ao = mix(1.0,
		SAMPLE_BINDLESS(uAOHandle, uv).r, uMaterial.aoStrength);

	/* --- Emissive --- */
	uv = get_uv_loc(uMaterial.emissiveTexCoord);
	vec3 emissive = uMaterial.emissiveFactor.rgb
		* SAMPLE_BINDLESS(uEmissiveHandle, uv).rgb;

	//FragColor = vec4(uMaterial.emissiveFactor.rgb, 1.0f); return;

	/* --- Debug Overlays --- */
	vec4 debugColor;
	if (apply_debug_channels(debugColor, albedo, N,
		roughness, metallic, ao, emissive))
	{
		FragColor = debugColor;
		return;
	}

	/* --- Lighting --- */
	vec3 V = normalize(vCamPos - vFragPos);

	vec3 color = compute_pbr(albedo.rgb, N, V,
		metallic, roughness, ao, emissive);

	/* --- Post Processing --- */
	color = tonemap_lottes(color, 2.0);
	color = apply_gamma(color, 2.2);

#if defined(DEBUG_IN)
	FragColor = vec4(vDebug, 1.0);
#else
	FragColor = vec4(color, albedo.a);
#endif
}
