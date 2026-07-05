#pragma once

#include <WinkEngine/GFX/Camera.hpp>
#include <WinkEngine/GFX/Renderer/Data.hpp>

namespace Wink::GFX
{
	void draw_skybox(const Camera& cam);
	void draw(const DrawData& drawData);
}