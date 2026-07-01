#version 460 core

#include "Tonemapping.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D uEquirect;
layout(rgba16f, binding = 1) uniform writeonly imageCube uCubemap;

const vec2 INV_ATAN = vec2(0.15915494, 0.31830989);

vec2 sample_equirect(vec3 dir)
{
	vec2 uv = vec2(atan(dir.z, dir.x),
		asin(clamp(dir.y, -1.0, 1.0)));
	uv *= INV_ATAN;
	uv += 0.5;
	return uv;
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

void main()
{
	ivec2 faceSize = imageSize(uCubemap);
	ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
	int face = int(gl_GlobalInvocationID.z);

	if (any(greaterThanEqual(texel, faceSize))) return;

	vec2 st = (vec2(texel) + 0.5) / vec2(faceSize) * 2.0 - 1.0;
	vec3 dir = face_dir(face, st);
	vec2 uv = sample_equirect(dir);

	vec3 color = texture(uEquirect, uv).rgb;
	color = min(color, 16);

	imageStore(uCubemap, ivec3(texel, face), vec4(color, 1.0));
}