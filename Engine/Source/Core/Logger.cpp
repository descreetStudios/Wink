#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Wink::Logger
{
	namespace
	{
		std::shared_ptr<spdlog::logger> gInternalLogger;
		std::shared_ptr<spdlog::logger> gClientLogger;
	} // anonymous namespace

	bool init()
	{
		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		gInternalLogger = std::make_shared<spdlog::logger>("Internal", consoleSink);
		gClientLogger = std::make_shared<spdlog::logger>("App", consoleSink);

		spdlog::register_logger(gInternalLogger);
		spdlog::register_logger(gClientLogger);
		spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");

		gInternalLogger->set_level(spdlog::level::info);
		gClientLogger->set_level(spdlog::level::info);

		return true;
	}

	void shutdown()
	{
		spdlog::shutdown();
	}

	void log(Mode mode, Level lvl, std::string_view msg)
	{
		auto& logger = (mode == Mode::Client) ? gClientLogger : gInternalLogger;
		assert(logger);
		logger->log(static_cast<spdlog::level::level_enum>(lvl), msg);
	}

	void set_level(Level lvl)
	{
		assert(gClientLogger);
		gClientLogger->set_level(static_cast<spdlog::level::level_enum>(lvl));
	}

	namespace Internal
	{
		void set_level(Level lvl)
		{
			assert(gInternalLogger);
			gInternalLogger->set_level(static_cast<spdlog::level::level_enum>(lvl));
		}
	}
}