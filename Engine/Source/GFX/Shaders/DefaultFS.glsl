#version 460 core

#include "ToneMapping.glsli"
#include "Material.glsli"
#include "PBR.glsli"
#include "Light.glsli"

// #define DEBUG_ALBEDO
// #define DEBUG_NORMALS
// #define DEBUG_ROUGHNESS
// #define DEBUG_METALLIC
// #define DEBUG_AO
// #define DEBUG_EMISSIVE

// #define DEBUG_IN

#include "Debug.glsli"

in vec3 vFragPos;
in vec2 vTexCoord;
in vec2 vTexCoord1;
in mat3 vTBN;
in vec3 vDebug;

out vec4 FragColor;

uniform vec3 uCamPos;

// ------------------------------------------------------------
// IBL
// ------------------------------------------------------------
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilteredMap;
uniform sampler2D uBRDFLUT;
uniform bool uHasIBL = false;

// ------------------------------------------------------------
// LIGHTING HELPERS
// ------------------------------------------------------------

vec3 compute_dir_light(DirLight light,
	vec3 albedo, vec3 N, vec3 V,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 L = normalize(-light.direction);
	float NdL = max(dot(N, L), 0.0);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = FresnelSchlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse = kD * albedo / PI;

	vec3 radiance = light.color * light.intensity;
	return (diffuse + specular) * radiance * NdL;
}

vec3 compute_point_light(PointLight light,
	vec3 albedo, vec3 N, vec3 V, vec3 fragPos,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 toLight = light.position - fragPos;
	float dist = length(toLight);

	float window = 1.0 - smoothstep(light.radius * 0.75, light.radius, dist);
	if (window <= 0.0) return vec3(0.0);

	vec3 L = toLight / dist;
	float NdL = max(dot(N, L), 0.0);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float attenuation = (light.intensity * window) / (dist * dist + 1.0);

	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = FresnelSchlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse = kD * albedo / PI;

	vec3 radiance = light.color * attenuation;
	return (diffuse + specular) * radiance * NdL;
}

vec3 compute_spot_light(SpotLight light,
	vec3 albedo, vec3 N, vec3 V, vec3 fragPos,
	float NdV, float metallic, float roughness, vec3 F0)
{
	vec3 toLight = light.position - fragPos;
	float dist = length(toLight);

	float window = 1.0 - smoothstep(light.range * 0.75, light.range, dist);
	if (window <= 0.0) return vec3(0.0);

	vec3 L = toLight / dist;
	float theta = dot(L, normalize(-light.direction));

	float epsilon = light.innerCutoff - light.outerCutoff;
	float spotIntensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
	if (spotIntensity <= 0.0) return vec3(0.0);

	float NdL = max(dot(N, L), 0.0);
	if (NdL <= 0.0) return vec3(0.0);

	vec3 H = normalize(L + V);
	float HdV = max(dot(H, V), 0.0);

	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = FresnelSchlick(HdV, F0);

	vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse = kD * albedo / PI;

	float attenuation = (light.intensity * window * spotIntensity) / (dist * dist + 1.0);
	vec3 radiance = light.color * attenuation;

	return (diffuse + specular) * radiance * NdL;
}

// ------------------------------------------------------------
// IBL AMBIENT
// ------------------------------------------------------------

vec3 compute_ibl(vec3 albedo, vec3 N, vec3 V,
	float NdV, float metallic, float roughness,
	float ao, vec3 F0)
{
	vec3 F = FresnelSchlickRoughness(NdV, F0, roughness);
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 irradiance = texture(uIrradianceMap, N).rgb * 0.3;
	vec3 diffuse = kD * irradiance * albedo;

	vec3 R = reflect(-V, N);
	float MAX_MIPS = float(textureQueryLevels(uPrefilteredMap));
	vec3 prefilteredColor = textureLod(uPrefilteredMap, R, roughness * MAX_MIPS).rgb * 0.7;
	vec2 brdf = texture(uBRDFLUT, vec2(NdV, roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	return (diffuse + specular) * ao;
}


// ------------------------------------------------------------
// PBR LIGHTING CORE
// ------------------------------------------------------------

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

	for (int i = 0; i < uPointLightCount; ++i)
		directLight += compute_point_light(uPointLights[i], albedo,
			N, V, vFragPos, NdV, metallic, roughness, F0);

	for (int i = 0; i < uSpotLightCount; ++i)
		directLight += compute_spot_light(uSpotLights[i], albedo,
			N, V, vFragPos, NdV, metallic, roughness, F0);

	vec3 ambient;
	if (uHasIBL) // TODO: Glitched
	{
		ambient = compute_ibl(albedo, N, V, NdV,
			metallic, roughness, ao, F0);
	}
	else ambient = albedo * F0 * (0.2 * ao);

	return directLight + ambient + emissive;
}

void main()
{
	// --------------------------------------------------------
	// MATERIAL STAGE
	// --------------------------------------------------------

	vec2 baseUV = (uMaterial.albedoTexCoord == 1 ? vTexCoord1 : vTexCoord);

	// Albedo
	vec4 albedo = uMaterial.baseColor;
	if (uMaterial.hasAlbedoMap)
		albedo *= texture(uMaterial.albedoMap, baseUV);

	// Normals
	vec3 N;
	if (uMaterial.hasNormalMap)
	{
		vec3 tsN = texture(uMaterial.normalMap, baseUV).rgb * 2.0 - 1.0;
		N = normalize(vTBN * tsN);
	}
	else N = normalize(vTBN[2]);

	// Metallic & Roughness
	float metallic  = uMaterial.metallic;
	float roughness = uMaterial.roughness;
	if (uMaterial.hasMRMap)
	{
		vec2 mr = texture(uMaterial.mrMap, baseUV).gb;
		roughness *= mr.x;
		metallic  *= mr.y;
	}

	// Ambient Occlusion
	float ao = 1.0;
	if (uMaterial.hasAOMap)
		ao = mix(1.0, texture(uMaterial.aoMap, baseUV).r, uMaterial.aoStrength);

	// Emissive
	vec3 emissive = uMaterial.emissiveFactor;
	if (uMaterial.hasEmissiveMap)
		emissive *= texture(uMaterial.emissiveMap, vTexCoord).rgb;

	// Debug Overlays
	vec4 debugColor;
	if (applyDebugChannels(debugColor, albedo, N,
		roughness, metallic, ao, emissive))
	{
		FragColor = debugColor;
		return;
	}

	// --------------------------------------------------------
    // LIGHTING STAGE
    // --------------------------------------------------------

	vec3 V = normalize(uCamPos - vFragPos);

	vec3 color = compute_pbr(albedo.rgb, N, V,
		metallic, roughness, ao, emissive);

	// --------------------------------------------------------
    // POST-PROCESSING STAGE
    // --------------------------------------------------------

	color = tonemap_uchimura(color);
	color = pow(color, vec3(1.0 / 2.2));

#if defined(DEBUG_IN)
	FragColor = vec4(vDebug, 1.0);
#else
	FragColor = vec4(color, albedo.a);
#endif
}
