#version 460 core

layout(location = 0) in vec3 aPos;

out vec3 vTexCoord;

uniform mat4 uViewProj;

void main()
{
    vTexCoord = aPos;
    vec4 clip = uViewProj * vec4(aPos, 1.0);

    gl_Position = clip.xyww;
}
