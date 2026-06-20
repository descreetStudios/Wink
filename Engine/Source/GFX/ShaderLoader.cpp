#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/ShaderLoader.hpp>
#include <GFX/Shader.hpp>

namespace Wink::GFX
{
	ShaderHandle ShaderLoader::load(
		const std::vector<ShaderSource>& sources)
	{
		auto program = Internal::create_program(sources);
		if (!program) return nullptr;
		return std::make_shared<ShaderProgram>(std::move(*program));
	}

	ShaderHandle ShaderLoader::load(
		const std::vector<ShaderFile>& files)
	{
		auto program = Internal::create_program(files);
		if (!program) return nullptr;
		return std::make_shared<ShaderProgram>(std::move(*program));
	}
}