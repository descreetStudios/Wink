#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material
{
    vec4 baseColor;
    float metallic;
    float roughness;
    vec3 emissiveFactor;
    float aoStrength;

    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D mrMap;
    sampler2D aoMap;
    sampler2D emissiveMap;

    int albedoTexCoord;
    int normalTexCoord;
    int mrTexCoord;
    int aoTexCoord;
    int emissiveTexCoord;
};

#endif // MATERIAL_GLSL