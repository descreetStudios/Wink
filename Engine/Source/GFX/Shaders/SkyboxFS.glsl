#version 460 core

in  vec3 vTexCoord;
out vec4 FragColor;

uniform samplerCube uSkybox;

void main()
{
    vec3 color = texture(uSkybox, vTexCoord).rgb;
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
