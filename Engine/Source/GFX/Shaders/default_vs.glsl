#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

out vec3 vFragPos;
out vec2 vTexCoord;
out mat3 vTBN;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
	vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos  = worldPos.xyz;
    vTexCoord = aTexCoord;

    mat3 normalMatrix = transpose(inverse(mat3(uModel)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent.xyz);
    T = normalize(T - dot(T, N) * N);       // re-orthogonalise (Gram-Schmidt)
    vec3 B = cross(N, T) * aTangent.w;      // w flips for mirrored UVs

    vTBN = mat3(T, B, N);

    gl_Position = uProj * uView * worldPos;
}
