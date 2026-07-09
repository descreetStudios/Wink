#version 460 core

in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uSceneColor;
uniform sampler2D uSceneDepth;

void main()
{
	vec3 color = texture(uSceneColor, vUV).rgb;

	// TODO: Add post-processing effects

	FragColor = vec4(color, 1.0);
}
