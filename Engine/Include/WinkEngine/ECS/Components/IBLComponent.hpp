#pragma once

#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink::ECS
{
	struct IBLComponent
	{
		GFX::IBL::IBLData iblData;
	};
}