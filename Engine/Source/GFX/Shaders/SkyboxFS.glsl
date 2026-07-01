#version 460 core

#include "ToneMapping.glsl"

in  vec3 vTexCoord;
out vec4 FragColor;

uniform samplerCube uSkybox;

void main()
{
    vec3 color = texture(uSkybox, vTexCoord).rgb;
    color = tonemap_lottes(color, 1.1);
    color = apply_gamma(color, 2.7);
    FragColor = vec4(color, 1.0);
}
