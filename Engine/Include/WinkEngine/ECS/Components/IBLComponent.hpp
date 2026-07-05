#pragma once

#include <WinkEngine/GFX/Renderer/IBL.hpp>

namespace Wink::ECS
{
	struct IBLComponent
	{
		GFX::IBL::IBLData iblData;
		bool skybox = true;
	};
}