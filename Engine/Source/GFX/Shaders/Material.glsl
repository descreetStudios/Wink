#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material
{
    vec4 baseColor;
    float metallic;
    float roughness;
    float aoStrength;
    float _pad;
    vec4 emissiveFactor; // .w unused

    int albedoTexCoord;
    int normalTexCoord;
    int mrTexCoord;
    int aoTexCoord;
    int emissiveTexCoord;
    int _pad1[3];
};

layout(std140, binding = 2) uniform MaterialUBO
{
    uvec2 uAlbedoHandle;
    uvec2 uNormalHandle;
    uvec2 uMRHandle;
    uvec2 uAOHandle;
    uvec2 uEmissiveHandle;
    uvec2 _pad3;

    Material uMaterial;
};

#define SAMPLE_BINDLESS(handle, uv) \
    texture(sampler2D(handle), uv)

#endif // MATERIAL_GLSL