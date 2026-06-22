#version 460 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 uCamPos;

struct Material
{
	vec4 baseColor;

	sampler2D albedoMap;
	bool hasAlbedoMap;

	sampler2D normalMap;
	bool hasNormalMap;
};
uniform Material uMaterial;

const vec3 LIGHT_POS = vec3(10.0, 20.0, 10.0);
const vec3 LIGHT_COLOR = vec3(1.0);
const float AMBIENT = 0.08;

void main()
{
	vec4 albedo = uMaterial.baseColor;
	if (uMaterial.hasAlbedoMap)
		albedo *= texture(uMaterial.albedoMap, vTexCoord);

	vec3 N = normalize(vNormal);
	vec3 L = normalize(LIGHT_POS - vFragPos);
	vec3 V = normalize(uCamPos - vFragPos);
	vec3 H = normalize(L + V);

	float diff = max(dot(N, L), 0.0);
	float spec = pow(max(dot(N, H), 0.0), 64.0);

	vec3 color = albedo.rgb * (AMBIENT + diff * LIGHT_COLOR)
		+ spec * LIGHT_COLOR * 0.5;

	// UV Check
	//FragColor = vec4(vTexCoord, 0.0, 1.0);

	// Normals check
	// FragColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);

	FragColor = vec4(color, albedo.a);
}
