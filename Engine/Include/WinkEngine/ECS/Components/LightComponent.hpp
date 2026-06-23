#pragma once

#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink::ECS
{
	struct DirLightComponent
	{
		GFX::DirLight dirLight;
	};

	struct PointLightComponent
	{
		GFX::PointLight pointLight;
	};

	struct SpotLightComponent
	{
		GFX::SpotLight spotLight;
	};
}