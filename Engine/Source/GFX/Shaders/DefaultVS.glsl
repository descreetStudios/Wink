#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec2 aTexCoord1;
layout (location = 4) in vec4 aTangent;

out vec3 vFragPos;
out vec3 vCamPos;
out mat4 vView;
out vec2 vTexCoord;
out vec2 vTexCoord1;
out mat3 vTBN;

flat out uint vTileCountX;

out vec3 vDebug;

uniform mat4 uModel;
uniform mat3 uNormalMatrix;

layout(std140, binding = 0) uniform FrameUBO 
{
    mat4 uView;
    mat4 uViewProj;
    vec3 uCamPos;
    uint uTileCountX;
};

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vCamPos = uCamPos;
    vView = uView;
    vTexCoord = aTexCoord;
    vTexCoord1 = aTexCoord1;

    vTileCountX = uTileCountX;

    vec3 N = normalize(uNormalMatrix * aNormal);
    vec3 T = normalize(uNormalMatrix * aTangent.xyz);
    vec3 B = cross(N, T) * aTangent.w;

    vTBN = mat3(T, B, N);
    vDebug = T;

    gl_Position = uViewProj * worldPos;
}