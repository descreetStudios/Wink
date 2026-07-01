#ifndef PBR_GLSL
#define PBR_GLSL

const float PI = 3.14159265359;

/* --- Trowbridge–Reitz GGX Microfacet Normal Distribution Function (Walter et al., 2007) --- */
float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdH = max(dot(N, H), 0.0);
    float d = (NdH * NdH) * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.0001);
}

/* --- Smith-GGX Geometry Function  (Smith, 1967) --- */
float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float NdV = max(dot(N, V), 0.0);
    float NdL = max(dot(N, L), 0.0);
    
    float ggx1 = NdV / (NdV * (1.0 - k) + k);
    float ggx2 = NdL / (NdL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

/* --- Schlick's Fresnel approximation (Schlick, 1994) --- */
vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) *
        pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/* --- Schlick Fresnel with roughness remapping (Burley, Disney BRDF, 2012) --- */
vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) *
        pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

#endif // PBR_GLSL