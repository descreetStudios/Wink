#version 460 core

#define TILE_SIZE 16

in vec2 vUV;
out vec4 FragColor;

uniform uint uTileCountX;
uniform uint uTileCountY;
uniform uint uScreenWidth;
uniform uint uScreenHeight;
uniform uint uMaxLightsPerTile;

layout(std430, binding = 0) readonly buffer LightGrid
{
	uvec2 oLightGrid[]; // .x = pointCount, .y = spotCount
};

vec3 heatmap(float t)
{
	t = clamp(t, 0.0, 1.0);
	vec3 cold = vec3(0.0, 0.0, 1.0); // blue
	vec3 mid = vec3(0.0, 1.0, 0.0); // green
	vec3 hot = vec3(1.0, 0.0, 0.0); // red

	if (t < 0.5)
		return mix(cold, mid, t * 2.0);
	else
		return mix(mid, hot, (t - 0.5) * 2.0);
}

void main()
{
	uvec2 pixelCoord = uvec2(gl_FragCoord.xy);
	uvec2 tileID = clamp(pixelCoord / uvec2(TILE_SIZE),
		uvec2(0), uvec2(uTileCountX - 1, uTileCountY - 1));
	uint tileIndex = tileID.y * uTileCountX + tileID.x;
	uvec2 grid = oLightGrid[tileIndex];
	uint total = grid.x + grid.y;

	uvec2 pixelInTile = pixelCoord % uvec2(TILE_SIZE);
	bool border = pixelInTile.x == 0u || pixelInTile.y == 0u;

	if (total == 0u) discard;
	else if (border) FragColor = vec4(0.2, 0.2, 0.2, 0.6);
	else
	{
		uint globalMax = 0u;
		for (uint i = 0u; i < uTileCountX * uTileCountY; ++i)
			globalMax = max(globalMax, oLightGrid[i].x + oLightGrid[i].y);

		float heat = float(total) / float(max(globalMax, 8u));
		FragColor  = vec4(heatmap(heat), 0.4);
	}

	//if (tileIndex % 2 == 0)
	//    FragColor = vec4(0.5, 0.0, 0.0, 0.2);
	//else
	//    FragColor = vec4(0.0, 0.0, 0.5, 0.2);
}