#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#define MAX_DIR_LIGHTS 2
#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8

struct DirLight
{
	vec3 direction;
	float intensity;
	vec3 color;
};

struct PointLight
{
    vec3 position;
    float intensity;
    vec3 color;
    float radius;
};

struct SpotLight
{
    vec3 position;
    float range;
    vec3 direction;
    float innerCutoff;
    vec3 color;
    float outerCutoff;
    float intensity;
};

uniform DirLight uDirLights[MAX_DIR_LIGHTS];
uniform uint uDirLightCount;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];
uniform uint uPointLightCount;
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];
uniform uint uSpotLightCount;

#endif // LIGHT_GLSL