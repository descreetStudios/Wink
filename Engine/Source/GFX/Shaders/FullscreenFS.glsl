#version 460 core

layout(binding = 0) uniform sampler2D uTex;

in vec2 vUV;
out vec4 FragColor;

void main()
{
    FragColor = texture(uTex, vUV);
}
