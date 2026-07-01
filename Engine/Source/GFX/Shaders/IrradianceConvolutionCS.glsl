#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube uEnvironment;
layout(rgba16f, binding = 1)  uniform writeonly imageCube uIrradiance;

const uint SAMPLE_COUNT = 2048u;
const float PI = 3.14159265359;

float radical_inverse_vdc(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint n)
{
	return vec2(float(i) / float(n), radical_inverse_vdc(i));
}

vec3 hemisphere_sample(vec2 xi)
{
	float phi = 2.0 * PI * xi.x;
	float cosTheta = sqrt(1.0 - xi.y);
	float sinTheta = sqrt(xi.y);
	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

vec3 face_dir(int face, vec2 st)
{
	switch (face)
	{
		case 0: return normalize(vec3( 1.0, -st.y, -st.x));
		case 1: return normalize(vec3(-1.0, -st.y,  st.x));
		case 2: return normalize(vec3( st.x,  1.0,  st.y));
		case 3: return normalize(vec3( st.x, -1.0, -st.y));
		case 4: return normalize(vec3( st.x, -st.y,  1.0));
		case 5: return normalize(vec3(-st.x, -st.y, -1.0));
	}
	return vec3(0.0);
}

mat3 make_tbn(vec3 n)
{
	vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 right = normalize(cross(up, n));
	vec3 fwd = cross(n, right);
	return mat3(right, fwd, n);
}

void main()
{
	ivec2 faceSize = imageSize(uIrradiance);
	ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
	int face = int(gl_GlobalInvocationID.z);

	if (any(greaterThanEqual(texel, faceSize))) return;

	vec2 st = (vec2(texel) + 0.5) / vec2(faceSize) * 2.0 - 1.0;
	vec3 n = face_dir(face, st);
	mat3 tbn = make_tbn(n);

	vec3 irradiance = vec3(0.0);

	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		vec2 xi = hammersley(i, SAMPLE_COUNT);
		vec3 localDir = hemisphere_sample(xi);
		vec3 worldDir = tbn * localDir;

		irradiance += textureLod(uEnvironment, worldDir, 2.0).rgb;
	}

	irradiance = PI * irradiance / float(SAMPLE_COUNT);
	imageStore(uIrradiance, ivec3(texel, face), vec4(irradiance, 1.0));
}
