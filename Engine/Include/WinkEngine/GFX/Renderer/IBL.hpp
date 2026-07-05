#pragma once

#include <WinkEngine/GFX/RES/Handle.hpp>

namespace Wink::GFX
{
	namespace IBL
	{
		struct IBLData
		{
			RES::CubemapHandle envMap;
			RES::CubemapHandle irradianceMap;
			RES::CubemapHandle prefilteredEnvMap;
		};

		[[nodiscard]] RES::CubemapHandle equirect_to_cubemap(
			RES::TextureHandle hdr, u32 faceSize = 512);
	}
}