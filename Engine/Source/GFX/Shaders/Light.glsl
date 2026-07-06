#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#define MAX_DIR_LIGHTS 2
#define MAX_POINT_LIGHTS 800
#define MAX_SPOT_LIGHTS 8

struct DirLight
{
    vec4 direction;  // .xyz = direction, .w = intensity
    vec4 color;      // .xyz = color
};

struct PointLight
{
    vec4 position;   // .xyz = position,  .w = intensity
    vec4 color;      // .xyz = color,     .w = radius
};

struct SpotLight
{
    vec4 position;   // .xyz = position,  .w = range
    vec4 direction;  // .xyz = direction, .w = intensity
    vec4 color;      // .xyz = color,     .w = outerCutoff
    vec4 inner;      // .x   = innerCutoff
};

layout(std140, binding = 1) uniform LightsUBO
{
    uint uDirLightCount;
    uint uPointLightCount;
    uint uSpotLightCount;
    uint _p1;

    DirLight uDirLights[MAX_DIR_LIGHTS];
    PointLight uPointLights[MAX_POINT_LIGHTS];
    SpotLight uSpotLights[MAX_SPOT_LIGHTS];
};

#endif // LIGHT_GLSL