#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#define MAX_DIR_LIGHTS 2

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
    uint _lp0;
    uint _lp1;
    uint _lp2;

    DirLight uDirLights[MAX_DIR_LIGHTS];
};

#endif // LIGHT_GLSL