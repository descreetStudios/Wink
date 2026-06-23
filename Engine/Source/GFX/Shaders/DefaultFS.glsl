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

#include "Debug.glsli"

in vec3 vFragPos;
in vec2 vTexCoord;
in vec2 vTexCoord1;
in mat3 vTBN;

in vec3 vDebug;

out vec4 FragColor;

uniform vec3 uCamPos;

vec2 get_uv(int texCoord)
{
    return texCoord == 1 ? vTexCoord1 : vTexCoord;
}

vec3 compute_dir_light(DirLight light,
    vec3 albedo, vec3 N, vec3 V,
    float metallic, float roughness, vec3 F0)
{
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(L + V);

    float NdL = max(dot(N, L), 0.0);
    float NdV = max(dot(N, V), 0.0);
    float HdV = max(dot(H, V), 0.0);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = FresnelSchlick(HdV, F0);

    vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);

    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    vec3 radiance = light.color * light.intensity;
    return (diffuse + specular) * radiance * NdL;
}

vec3 compute_pbr(vec3 albedo, vec3 N, vec3 V,
    float metallic, float roughness,
    float ao, vec3 emissive)
{
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Accumulate directional lights
    vec3 directLight = vec3(0.0);
    for (int i = 0; i < uDirLightCount; ++i)
        directLight += compute_dir_light(uDirLights[i],
        albedo, N, V, metallic, roughness, F0);

    // Approximate ambient
    vec3 ambient = albedo * F0 * 0.15 * ao;

    vec3 color = directLight + ambient + emissive;
    return color;
}

void main()
{
    // Albedo
    vec4 albedo = uMaterial.baseColor;
    if (uMaterial.hasAlbedoMap)
        albedo *= texture(uMaterial.albedoMap, get_uv(uMaterial.albedoTexCoord));

    // Normals
    vec3 N;
    if (uMaterial.hasNormalMap)
    {
        vec3 tsN = texture(uMaterial.normalMap, get_uv(uMaterial.albedoTexCoord)).rgb * 2.0 - 1.0;
        N = normalize(vTBN * tsN);
    }
    else 
    {
        N = normalize(vTBN[2]);
    }

    // Metallic & Roughness
    float metallic  = uMaterial.metallic;
    float roughness = uMaterial.roughness;
    if (uMaterial.hasMRMap)
    {
        vec2 mr = texture(uMaterial.mrMap, get_uv(uMaterial.albedoTexCoord)).gb; // G = roughness, B = metallic
        roughness *= mr.x;
        metallic  *= mr.y;
    }

    // Ambient Occlusion
    float ao = 1.0;
    if (uMaterial.hasAOMap)
        ao = mix(1.0, texture(uMaterial.aoMap, get_uv(uMaterial.albedoTexCoord)).r, uMaterial.aoStrength);

    // Emissive
    vec3 emissive = uMaterial.emissiveFactor;
    if (uMaterial.hasEmissiveMap)
        emissive *= texture(uMaterial.emissiveMap, vTexCoord).rgb;

    // Debug Overlays
    vec4 debugColor;
    if (applyDebugChannels(debugColor, albedo, N, roughness, metallic, ao, emissive))
    {
        FragColor = debugColor;
        return;
    }

    vec3 V = normalize(uCamPos - vFragPos);

    vec3 color = compute_pbr(albedo.rgb, N, V,
        metallic, roughness, ao, emissive);

    // Post Processing
    color = tonemap_uchimura(color);
    color = pow(color, vec3(1.0 / 2.2));

    // FragColor = vec4(vDebug, 1.0);

    FragColor = vec4(color, albedo.a);
}