#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Shader.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	namespace
	{
		std::string_view shader_type_to_string(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::Vertex: return "Vertex";
			case ShaderType::Fragment: return "Fragment";
			case ShaderType::Geometry: return "Geometry";
			case ShaderType::TessControl: return "TessControl";
			case ShaderType::TessEvaluation: return "TessEvaluation";
			case ShaderType::Compute: return "Compute";
			default: return "Unknown";
			}
		}

		std::string read_info_log(u32 id, bool isProgram)
		{
			i32 logLen = 0;

			if (isProgram) glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLen);
			else glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);

			if (logLen <= 0)
				return {};

			std::string log;
			log.resize(static_cast<u32>(logLen));

			if (isProgram) glGetProgramInfoLog(id, logLen, nullptr, log.data());
			else glGetShaderInfoLog(id, logLen, nullptr, log.data());

			if (!log.empty() && log.back() == '\0')
				log.pop_back();

			return log;
		}
	} // anonymous namespace

	ShaderStage::ShaderStage(ShaderType type)
		: mType(type)
	{
		mID = glCreateShader(static_cast<u32>(type));
		assert(mID != 0);
	}

	ShaderStage::~ShaderStage()
	{
		glDeleteShader(mID);
	}

	MOVE_CTOR_IMPL(ShaderStage) noexcept
		: mID(o.mID), mType(o.mType)
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(ShaderStage) noexcept
	{
		if (this != &o)
		{
			glDeleteShader(mID);
			mID = o.mID;
			mType = o.mType;
			o.mID = 0;
		}
		return *this;
	}

	bool ShaderStage::compile(std::string_view src) const
	{
		assert(is_valid());

		const char* ptr = src.data();
		const i32 len = static_cast<i32>(src.size());
		glShaderSource(mID, 1, &ptr, &len);
		glCompileShader(mID);

		i32 ok = 0;
		glGetShaderiv(mID, GL_COMPILE_STATUS, &ok);

		if (!ok)
		{
			Logger::Internal::error(
				"Shader compilation error in '{}' shader:\n{}",
				shader_type_to_string(mType),
				read_info_log(mID, false)
			);
			return false;
		}

		return true;
	}

	ShaderProgram::ShaderProgram()
	{
		mID = glCreateProgram();
		assert(mID != 0);
	}

	ShaderProgram::~ShaderProgram()
	{
		glDeleteProgram(mID);
	}

	MOVE_CTOR_IMPL(ShaderProgram) noexcept
		: mID(o.mID), mUniformCache(std::move(o.mUniformCache))
	{
		o.mID = 0;
	}

	MOVE_ASSIGN_IMPL(ShaderProgram) noexcept
	{
		if (this != &o)
		{
			glDeleteProgram(mID);
			mID = o.mID;
			mUniformCache = std::move(o.mUniformCache);
			o.mID = 0;
		}
		return *this;
	}

	void ShaderProgram::use() const noexcept
	{
		assert(is_valid());
		glUseProgram(mID);
	}

	void ShaderProgram::unuse() noexcept
	{
		glUseProgram(0);
	}

	void ShaderProgram::dispatch(u32 numGroupsX,
		u32 numGroupsY, u32 numGroupsZ) noexcept
	{
		assert(numGroupsX > 0 && numGroupsY > 0 && numGroupsZ > 0);
		glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
	}

	void ShaderProgram::memory_barrier(u32 barriers) noexcept
	{
		glMemoryBarrier(barriers);
	}

	i32 ShaderProgram::loc(std::string_view name) const noexcept
	{
		auto it = mUniformCache.find(std::string(name));
		if (it != mUniformCache.end())
			return it->second;

		std::string key(name);
		i32 l = glGetUniformLocation(mID, key.c_str());
		mUniformCache.emplace(std::move(key), l);
		return l;
	}

	void ShaderProgram::set(
		std::string_view name, bool v) const noexcept
	{
		assert(is_valid());
		glProgramUniform1i(mID, loc(name), v ? 1 : 0);
	}

	void ShaderProgram::set(
		std::string_view name, i32 v) const noexcept
	{
		assert(is_valid());
		glProgramUniform1i(mID, loc(name), v);
	}

	void ShaderProgram::set(
		std::string_view name, u32 v) const noexcept
	{
		assert(is_valid());
		glProgramUniform1ui(mID, loc(name), v);
	}

	void ShaderProgram::set(
		std::string_view name, float v) const noexcept
	{
		assert(is_valid());
		glProgramUniform1f(mID, loc(name), v);
	}

	void ShaderProgram::set(
		std::string_view name, glm::vec2 v) const noexcept
	{
		assert(is_valid());
		glProgramUniform2fv(mID, loc(name), 1, glm::value_ptr(v));
	}

	void ShaderProgram::set(
		std::string_view name, glm::vec3 v) const noexcept
	{
		assert(is_valid());
		glProgramUniform3fv(mID, loc(name), 1, glm::value_ptr(v));
	}

	void ShaderProgram::set(
		std::string_view name, glm::vec4 v) const noexcept
	{
		assert(is_valid());
		glProgramUniform4fv(mID, loc(name), 1, glm::value_ptr(v));
	}

	void ShaderProgram::set(
		std::string_view name, const glm::mat3& v) const noexcept
	{
		assert(is_valid());
		glProgramUniformMatrix3fv(mID, loc(name), 1,
			GL_FALSE, glm::value_ptr(v));
	}

	void ShaderProgram::set(std::string_view name,
		const glm::mat4& v) const noexcept
	{
		assert(is_valid());
		glProgramUniformMatrix4fv(mID, loc(name), 1,
			GL_FALSE, glm::value_ptr(v));
	}

	void ShaderProgram::bind_ubo_block(
		std::string_view blockName, u32 bindingPoint) const noexcept
	{
		assert(is_valid());

		std::string name(blockName);
		u32 idx = glGetUniformBlockIndex(mID, name.c_str());

		if (idx != GL_INVALID_INDEX)
			glUniformBlockBinding(mID, idx, bindingPoint);
	}

	namespace Internal
	{
		namespace
		{
			const fs::path SHADER_ROOT_DIR = "Shaders";

			std::string load_shader_source(const fs::path& path)
			{
				if (!fs::exists(path))
				{
					Logger::Internal::error(
						"Shader file does not exist: '{}'",
						path.string());
					return {};
				}

				if (!fs::is_regular_file(path))
				{
					Logger::Internal::error(
						"Shader path is not a file: '{}'",
						path.string());
					return {};
				}

				std::ifstream file(path);
				if (!file.is_open())
				{
					Logger::Internal::error(
						"Failed to open shader file: '{}'",
						path.string());
					return {};
				}

				std::stringstream ss;
				ss << file.rdbuf();
				return ss.str();
			}

			bool paths_equivalent(const fs::path& a, const fs::path& b)
			{
				std::error_code ec;
				bool result = fs::equivalent(a, b, ec);
				return !ec && result;
			}

			bool parse_include_directive(
				const std::string& line, std::string& outPath)
			{
				size_t i = 0;
				while (i < line.size() && std::isspace(
					static_cast<unsigned char>(line[i])))
					++i;

				static constexpr std::string_view kDirective = "#include";
				if (line.compare(i, kDirective.size(), kDirective) != 0)
					return false;

				i += kDirective.size();
				while (i < line.size() && std::isspace(
					static_cast<unsigned char>(line[i])))
					++i;

				if (i >= line.size())
					return false;

				const char open = line[i];
				const char close = (open == '<') ? '>' : '"';
				if (open != '"' && open != '<')
					return false;

				++i;
				const size_t start = i;
				while (i < line.size() && line[i] != close)
					++i;

				if (i >= line.size())
					return false;

				outPath = line.substr(start, i - start);
				return true;
			}

			fs::path resolve_include_path(
				const std::string& includeText,
				const fs::path& includerDir)
			{
				fs::path relativeToFile = includerDir / includeText;
				if (fs::exists(relativeToFile))
					return relativeToFile;

				fs::path relativeToRoot = SHADER_ROOT_DIR / includeText;
				if (fs::exists(relativeToRoot))
					return relativeToRoot;

				return {};
			}

			i32 source_string_index(
				const fs::path& path,
				std::vector<fs::path>& fileTable)
			{
				for (size_t n = 0; n < fileTable.size(); ++n)
				{
					if (paths_equivalent(fileTable[n], path))
						return static_cast<i32>(n);
				}
 
				fileTable.push_back(path);
				return static_cast<i32>(fileTable.size() - 1);
			}

			bool preprocess_includes(
				const fs::path& path,
				std::vector<fs::path>& fileTable,
				std::vector<fs::path>& includeStack,
				std::string& outResult,
				bool isTopLevel = true)
			{
				for (const fs::path& active : includeStack)
				{
					if (paths_equivalent(active, path))
					{
						Logger::Internal::error(
							"Circular #include detected involving '{}'",
							path.string());
						return false;
					}
				}

				std::string raw = load_shader_source(path);
				if (raw.empty())
					return false;

				const i32 selfIndex = source_string_index(path, fileTable);
				const fs::path selfDir = path.parent_path();

				includeStack.push_back(path);

				std::istringstream stream(raw);
				std::string line;
				i32 lineNumber = 0;
				bool ok = true;
				bool versionEmitted = false;

				while (std::getline(stream, line))
				{
					++lineNumber;

					if (isTopLevel && lineNumber == 1)
					{
						size_t i = 0;
						while (i < line.size() && std::isspace(
							static_cast<unsigned char>(line[i])))
							++i;

						static constexpr std::string_view VERSION_DIRECTIVE = "#version";
						if (line.compare(i, VERSION_DIRECTIVE.size(), VERSION_DIRECTIVE) == 0)
						{
							outResult += line;
							outResult += '\n';
							versionEmitted = true;

							outResult += "#line ";
							outResult += std::to_string(lineNumber + 1);
							outResult += ' ';
							outResult += std::to_string(selfIndex);
							outResult += '\n';
							continue;
						}
					}

					if (lineNumber == 1 && !versionEmitted)
					{
						outResult += "#line 1 ";
						outResult += std::to_string(selfIndex);
						outResult += '\n';
					}

					std::string includeText;
					if (parse_include_directive(line, includeText))
					{
						fs::path resolved = resolve_include_path(includeText, selfDir);
						if (resolved.empty())
						{
							Logger::Internal::error(
								"Could not resolve #include \"{}\" from '{}' "
								"(checked relative to file and to '{}')",
								includeText, path.string(),
								SHADER_ROOT_DIR.string());
							ok = false;
							break;
						}

						if (!preprocess_includes(
							resolved, fileTable, includeStack, outResult, false))
						{
							ok = false;
							break;
						}

						outResult += "#line ";
						outResult += std::to_string(lineNumber + 1);
						outResult += ' ';
						outResult += std::to_string(selfIndex);
						outResult += '\n';
					}
					else
					{
						outResult += line;
						outResult += '\n';
					}
				}

				includeStack.pop_back();
				return ok;
			}

			std::string load_shader_source_with_includes(
				const fs::path& path,
				std::vector<fs::path>& outFileTable)
			{
				std::vector<fs::path> includeStack;
				std::string result;

				if (!preprocess_includes(path,
					outFileTable, includeStack, result))
					return {};

				return result;
			}

			bool link_program(u32 programID)
			{
				glLinkProgram(programID);

				i32 ok = 0;
				glGetProgramiv(programID, GL_LINK_STATUS, &ok);
				if (!ok)
				{
					Logger::Internal::error("Shader linking error: '{}'",
						read_info_log(programID, true));
					return false;
				}

				return true;
			}
		} // anonymous namespace

		std::optional<ShaderProgram> create_program(
			const std::vector<ShaderSource>& sources)
		{
			assert(!sources.empty());

			std::vector<ShaderStage> stages;
			stages.reserve(sources.size());

			for (const ShaderSource& s : sources)
			{
				ShaderStage stage(s.type);
				if (!stage.compile(s.source))
					return std::nullopt;

				stages.emplace_back(std::move(stage));
			}

			ShaderProgram program;

			for (const ShaderStage& s : stages)
				glAttachShader(program.get_id(), s.get_id());

			if (!link_program(program.get_id()))
				return std::nullopt;

			for (const ShaderStage& s : stages)
				glDetachShader(program.get_id(), s.get_id());

			return program;
		}

		std::optional<ShaderProgram> create_program(
			const std::vector<ShaderFile>& files)
		{
			assert(!files.empty());

			std::vector<ShaderSource> sources;
			sources.reserve(files.size());

			std::vector<fs::path> fileTable;

			for (const ShaderFile& f : files)
			{
				std::string src = load_shader_source_with_includes(f.path, fileTable);
				if (src.empty())
					return std::nullopt;

				sources.push_back({ f.type, std::move(src) });
			}

			auto program = create_program(sources);
			if (!program && fileTable.size() > 1)
			{
				std::string mapping;
				for (size_t n = 0; n < fileTable.size(); ++n)
				{
					mapping += "\n  [";
					mapping += std::to_string(n);
					mapping += "] ";
					mapping += fileTable[n].string();
				}

				Logger::Internal::error(
					"Shader #line source map:{}", mapping);
			}

			return program;
		}
	}
}