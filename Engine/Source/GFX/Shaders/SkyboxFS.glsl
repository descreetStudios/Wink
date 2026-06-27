#version 460 core

#include "ToneMapping.glsli"

in  vec3 vTexCoord;
out vec4 FragColor;

uniform samplerCube uSkybox;
uniform float uExposure = 0.7;

void main()
{
    vec3 color = texture(uSkybox, vTexCoord).rgb;

    color *= uExposure;
    color = tonemap_uchimura(color);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
