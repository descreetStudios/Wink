#version 460 core

uniform sampler2D uDepth;
uniform float uNear;
uniform float uFar;

out vec4 FragColor;

float linearize(float d)
{
	return (2.0 * uNear) / (uFar + uNear - d * (uFar - uNear));
}

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2(textureSize(uDepth, 0));
	float d = texture(uDepth, uv).r;
	float linear = linearize(d);

	float near = 0.0;
	float far = 0.1;
	float remapped = clamp((linear - near) / (far - near), 0.0, 1.0);

	FragColor = vec4(vec3(remapped), 1.0);
}