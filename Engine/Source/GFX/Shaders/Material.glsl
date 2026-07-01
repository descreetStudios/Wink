#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material
{
    vec4 baseColor;
    float metallic;
    float roughness;
    vec3 emissiveFactor;
    float aoStrength;

    sampler2D albedoMap;    bool hasAlbedoMap;
    sampler2D normalMap;    bool hasNormalMap;
    sampler2D mrMap;        bool hasMRMap;
    sampler2D aoMap;        bool hasAOMap;
    sampler2D emissiveMap;  bool hasEmissiveMap;

    int albedoTexCoord;
    int normalTexCoord;
    int mrTexCoord;
    int aoTexCoord;
    int emissiveTexCoord;
};

#endif // MATERIAL_GLSL