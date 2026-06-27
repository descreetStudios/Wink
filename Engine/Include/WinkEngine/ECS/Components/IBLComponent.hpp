#pragma once

#include <WinkEngine/GFX/Resource/Handle.hpp>

namespace Wink::ECS
{
	struct IBLComponent
	{
		GFX::Resource::CubemapHandle cubemap;
	};
}