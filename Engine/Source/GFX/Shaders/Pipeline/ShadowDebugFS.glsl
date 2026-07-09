#version 460 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uShadowMap;

void main()
{
    float depth = texture(uShadowMap, vUV).r;
    FragColor = vec4(vec3(depth), 1.0);
}
