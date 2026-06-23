#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec2 aTexCoord1;
layout (location = 4) in vec4 aTangent;

out vec3 vFragPos;
out vec2 vTexCoord;
out vec2 vTexCoord1;
out mat3 vTBN;

out vec3 debug;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos  = worldPos.xyz;
    vTexCoord = aTexCoord;
    vTexCoord1 = aTexCoord1;

    mat3 normalMatrix = transpose(inverse(mat3(uModel)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent.xyz);
    
    // Gram-Schmidt re-orthogonalization
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * aTangent.w;

    vTBN = mat3(T, B, N);

    debug = T;

    gl_Position = uProj * uView * worldPos;
}