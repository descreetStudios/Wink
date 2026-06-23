#version 460 core

#include "ToneMapping.glsli"
#include "Material.glsli"
#include "PBR.glsli"

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

const vec3 LIGHT_POS = vec3(10.0, 20.0, 10.0);
const vec3 LIGHT_COLOR = vec3(1.0);

vec2 get_uv(int texCoord)
{
    return texCoord == 1 ? vTexCoord1 : vTexCoord;
}

vec3 compute_pbr(vec3 albedo, vec3 N, vec3 V,
    vec3 fragPos, float metallic, float roughness,
    float ao, vec3 emissive)
{
    // Light setup
    vec3 L = normalize(LIGHT_POS - fragPos);
    vec3 H = normalize(L + V);

    float NdL = max(dot(N, L), 0.0);
    float NdV = max(dot(N, V), 0.0);
    float HdV = max(dot(H, V), 0.0);

    // Fresnel base reflectance
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // BRDF terms
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = FresnelSchlick(HdV, F0);

    // Specular
    vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 0.001);

    // Diffuse
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    // Direct lighting
    vec3 color = (diffuse + specular) * LIGHT_COLOR * NdL;

    // Ambient approximation
    color += albedo * F0 * 0.3 * ao;

    // Emissive
    color += emissive;

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
        vFragPos, metallic, roughness, ao, emissive);

    // Post Processing
    color = tonemap_uchimura(color);
    color = pow(color, vec3(1.0 / 2.2));

    // FragColor = vec4(vDebug, 1.0);

    FragColor = vec4(color, albedo.a);
}