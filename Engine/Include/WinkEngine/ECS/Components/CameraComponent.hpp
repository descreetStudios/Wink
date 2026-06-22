#pragma once

#include <WinkEngine/GFX/Camera.hpp>

namespace Wink::ECS
{
	struct CameraComponent
	{
		GFX::Camera& camera;
	};
}