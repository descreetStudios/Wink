#define APP_APP 0
#define APP_EVENTS 0
#define APP_ECS 0
#define APP_RESOURCES 0
#define APP_MODELS 1

#if APP_APP
#include "Apps/App.hpp"
#elif APP_EVENTS
#include "Apps/EventsApp.hpp"
#elif APP_ECS
#include "Apps/ECSApp.hpp"
#elif APP_RESOURCES
#include "Apps/ResourcesApp.hpp"
#elif APP_MODELS
#include "Apps/ModelsApp.hpp"
#else
#error No apps enabled!
#endif

std::unique_ptr<Wink::Application> create_application()
{
	return std::make_unique<SandboxApp>();
}