#define APP_APP 0
#define APP_EVENTS 1

#if APP_APP
#include "Apps/App.hpp"
#elif APP_EVENTS
#include "Apps/EventsApp.hpp"
#else
#error No apps enabled!
#endif

std::unique_ptr<Wink::Application> create_application()
{
	return std::make_unique<SandboxApp>();
}