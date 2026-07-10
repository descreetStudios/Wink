#version 460 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2DArray uShadowMap;
uniform int uCascadeIndex;

void main()
{
    float depth = texture(uShadowMap, vec3(vUV, uCascadeIndex)).r;
    FragColor = vec4(vec3(depth), 1.0);
}
