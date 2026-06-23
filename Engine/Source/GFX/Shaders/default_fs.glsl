#version 460 core

in vec3 vFragPos;
in vec2 vTexCoord;
in mat3 vTBN;

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

// Uchimura 2017 — GT Tone Mapping
vec3 tonemap_uchimura(vec3 x)
{
    const float P = 1.0;   // max brightness
    const float a = 1.0;   // contrast
    const float m = 0.22;  // linear start
    const float l = 0.4;   // linear length
    const float c = 1.33;  // black tightness
    const float b = 0.0;   // pedestal
    float l0 = ((P-m)*l) / a;
    float L0 = m - m/a;
    float L1 = m + (1.0-m)/a;
    float S0 = m + l0;
    float S1 = m + a*l0;
    float C2 = (a*P) / (P - S1);
    float CP = -C2/P;
    vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
    vec3 w2 = vec3(step(m+l0, x));
    vec3 w1 = vec3(1.0) - w0 - w2;
    vec3 T  = vec3(m*pow(x/m, vec3(c)) + b);
    vec3 S  = vec3(P - (P-S1)*exp(CP*(x-S0)));
    vec3 L  = vec3(m + a*(x-m));
    return T*w0 + L*w1 + S*w2;
}

void main()
{
	vec4 albedo = uMaterial.baseColor;
	if (uMaterial.hasAlbedoMap)
		albedo *= texture(uMaterial.albedoMap, vTexCoord);

	vec3 N;
    if (uMaterial.hasNormalMap)
    {
        vec3 tsNormal = texture(uMaterial.normalMap, vTexCoord).rgb;
        tsNormal = tsNormal * 2.0 - 1.0;
        N = normalize(vTBN * tsNormal);
    }
    else N = normalize(vTBN[2]);

	vec3 L = normalize(LIGHT_POS - vFragPos);
	vec3 V = normalize(uCamPos - vFragPos);
	vec3 H = normalize(L + V);

	float diff = max(dot(N, L), 0.0);
	float spec = pow(max(dot(N, H), 0.0), 64.0);

	vec3 color = albedo.rgb * (AMBIENT + diff * LIGHT_COLOR)
		+ spec * LIGHT_COLOR * 0.5;

	color = tonemap_uchimura(color);
	color = pow(color, vec3(1.0 / 2.2));

	// UV Check
	// FragColor = vec4(vTexCoord, 0.0, 1.0);

	// TBN check
	// FragColor = vec4(vTBN[2] * 0.5 + 0.5, 1.0);

	FragColor = vec4(color, albedo.a);
}
