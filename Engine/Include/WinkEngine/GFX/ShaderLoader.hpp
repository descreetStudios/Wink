#pragma once

namespace Wink::GFX
{
	class ShaderProgram;

	enum class ShaderType : u32
	{
		Vertex = GL_VERTEX_SHADER,
		Fragment = GL_FRAGMENT_SHADER,
		Geometry = GL_GEOMETRY_SHADER,
		TessControl = GL_TESS_CONTROL_SHADER,
		TessEvaluation = GL_TESS_EVALUATION_SHADER,
		Compute = GL_COMPUTE_SHADER
	};

	struct ShaderFile
	{
		ShaderType type;
		fs::path path;
	};

	struct ShaderSource
	{
		ShaderType type;
		std::string source;
	};

	using ShaderHandle = std::shared_ptr<ShaderProgram>;
	using ShaderCache = entt::resource_cache<ShaderProgram>;

	struct ShaderLoader final :
		entt::resource_loader<ShaderProgram>
	{
		ShaderHandle load(const std::vector<ShaderSource>& sources);
		ShaderHandle load(const std::vector<ShaderFile>& files);
	};
}