#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
	vec4 worldPos = uModel * vec4(aPos, 1.0);
	vFragPos = worldPos.xyz;
	vNormal = normalize(transpose(inverse(mat3(uModel))) * aNormal);
	vTexCoord = aTexCoord;

	gl_Position = uProj * uView * worldPos;
}
