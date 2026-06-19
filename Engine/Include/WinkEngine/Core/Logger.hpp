#pragma once

namespace Wink::Logger
{
	enum class Mode { Client, Internal };
	enum class Level : i32
	{
		Trace, Debug, Info,
		Warn, Error, Critical,
		Off
	};

	bool init();
	void shutdown();

	void log(Mode mode, Level lvl, std::string_view msg);

	template<typename... Args>
	void log(Mode mode, Level lvl,
		std::format_string<Args...> fmt, Args&&... args)
	{
		log(mode, lvl, std::format(fmt, std::forward<Args>(args)...));
	}

	void set_level(Level lvl);

#pragma region Client Helpers

	template<typename... Args> void trace(std::format_string<Args...> fmt, Args&&... args)
	{
		log(Mode::Client, Level::Trace, fmt, std::forward<Args>(args)...);
	}
	template<typename... Args> void debug(std::format_string<Args...> fmt, Args&&... args)
	{
#ifdef WINK_DEBUG
		log(Mode::Client, Level::Debug, fmt, std::forward<Args>(args)...);
#endif
	}
	template<typename... Args> void info(std::format_string<Args...> fmt, Args&&... args)
	{
		log(Mode::Client, Level::Info, fmt, std::forward<Args>(args)...);
	}
	template<typename... Args> void warn(std::format_string<Args...> fmt, Args&&... args)
	{
		log(Mode::Client, Level::Warn, fmt, std::forward<Args>(args)...);
	}
	template<typename... Args> void error(std::format_string<Args...> fmt, Args&&... args)
	{
		log(Mode::Client, Level::Error, fmt, std::forward<Args>(args)...);
	}
	template<typename... Args> void critical(std::format_string<Args...> fmt, Args&&... args)
	{
		log(Mode::Client, Level::Critical, fmt, std::forward<Args>(args)...);
		assert(false);
	}

#pragma endregion

	namespace Internal
	{
		void set_level(Level lvl);

#pragma region Internal Helpers

		template<typename... Args> void trace(std::format_string<Args...> fmt, Args&&... args)
		{
			log(Mode::Internal, Level::Trace, fmt, std::forward<Args>(args)...);
		}
		template<typename... Args> void debug(std::format_string<Args...> fmt, Args&&... args)
		{
#ifdef WINK_DEBUG
			log(Mode::Internal, Level::Debug, fmt, std::forward<Args>(args)...);
#endif
		}
		template<typename... Args> void info(std::format_string<Args...> fmt, Args&&... args)
		{
			log(Mode::Internal, Level::Info, fmt, std::forward<Args>(args)...);
		}
		template<typename... Args> void warn(std::format_string<Args...> fmt, Args&&... args)
		{
			log(Mode::Internal, Level::Warn, fmt, std::forward<Args>(args)...);
		}
		template<typename... Args> void error(std::format_string<Args...> fmt, Args&&... args)
		{
			log(Mode::Internal, Level::Error, fmt, std::forward<Args>(args)...);
		}
		template<typename... Args> void critical(std::format_string<Args...> fmt, Args&&... args)
		{
			log(Mode::Internal, Level::Critical, fmt, std::forward<Args>(args)...);
			assert(false);
		}

#pragma endregion
	}
}