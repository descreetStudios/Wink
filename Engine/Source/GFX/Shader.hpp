#pragma once

namespace Wink::GFX
{
	enum class ShaderType : u32;
	struct ShaderFile;
	struct ShaderSource;

	class ShaderStage
	{
	public:
		explicit ShaderStage(ShaderType type);
		~ShaderStage();

		DISABLE_COPY(ShaderStage);
		MOVE_CTOR(ShaderStage) noexcept;
		MOVE_ASSIGN(ShaderStage) noexcept;

		bool compile(std::string_view src) const;

		[[nodiscard]] u32 get_id() const noexcept { return mID; }
		[[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

		operator bool() const noexcept { return is_valid(); }

	private:
		u32 mID = 0;
		ShaderType mType;
	};

	class ShaderProgram
	{
	public:
		ShaderProgram();
		~ShaderProgram();

		DISABLE_COPY(ShaderProgram);
		MOVE_CTOR(ShaderProgram) noexcept;
		MOVE_ASSIGN(ShaderProgram) noexcept;

		void use() const noexcept;
		static void unuse() noexcept;

		void set(std::string_view name, bool v) const noexcept;
		void set(std::string_view name, i32 v) const noexcept;
		void set(std::string_view name, u32 v) const noexcept;
		void set(std::string_view name, float v) const noexcept;
		void set(std::string_view name, glm::vec2 v) const noexcept;
		void set(std::string_view name, glm::vec3 v) const noexcept;
		void set(std::string_view name, glm::vec4 v) const noexcept;
		void set(std::string_view name, const glm::mat3& v) const noexcept;
		void set(std::string_view name, const glm::mat4& v) const noexcept;

		void set_texture(std::string_view name, i32 unit) const noexcept;
		void bind_ubo_block(std::string_view blockName, u32 bindingPoint) const noexcept;

		[[nodiscard]] u32 get_id() const noexcept { return mID; }
		[[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

		explicit operator bool() const noexcept { return is_valid(); }

	private:
		[[nodiscard]] i32 loc(std::string_view name) const noexcept;

	private:
		u32 mID = 0;
		mutable std::unordered_map<std::string, i32> mUniformCache;
	};

	namespace Internal
	{
		std::optional<ShaderProgram> create_program(const std::vector<ShaderSource>& sources);
		std::optional<ShaderProgram> create_program(const std::vector<ShaderFile>& files);
	}
}