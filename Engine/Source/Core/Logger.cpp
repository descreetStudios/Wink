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
		auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		if (!sink) return false;

		gInternalLogger = std::make_shared<spdlog::logger>("Internal", sink);
		gClientLogger = std::make_shared<spdlog::logger>("App", sink);

		if (!gInternalLogger || !gClientLogger)
			return false;

		spdlog::register_logger(gInternalLogger);
		spdlog::register_logger(gClientLogger);

		spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");

		return true;
	}

	void shutdown()
	{
		spdlog::shutdown();
		gInternalLogger.reset();
		gClientLogger.reset();
	}

	void log(Mode mode, Level lvl, std::string_view msg)
	{
		auto& logger = (mode == Mode::Client) ? gClientLogger : gInternalLogger;
		if (!logger) return;
		logger->log(static_cast<spdlog::level::level_enum>(lvl), msg);
	}

	void set_level(Level lvl)
	{
		if (!gClientLogger) return;
		gClientLogger->set_level(static_cast<spdlog::level::level_enum>(lvl));
	}

	namespace Internal
	{
		void set_level(Level lvl)
		{
			if (!gInternalLogger) return;
			gInternalLogger->set_level(static_cast<spdlog::level::level_enum>(lvl));
		}
	}
}