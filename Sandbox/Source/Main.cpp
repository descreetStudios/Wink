#define APP_APP 0
#define APP_EVENTS 0
#define APP_ECS 0
#define APP_RESOURCES 0
#define APP_MODELS 0
#define APP_IBL 0
#define APP_SCENE 0
#define APP_FORWARD_PLUS 1
#define APP_SHADOWS 1

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
#elif APP_IBL
#include "Apps/IBLApp.hpp"
#elif APP_SCENE
#include "Apps/SceneApp.hpp"
#elif APP_FORWARD_PLUS
#include "Apps/ForwardPlusApp.hpp"
#elif APP_SHADOWS
#include "Apps/ShadowsApp.hpp"
#else
#error No apps enabled!
#endif

std::unique_ptr<Wink::Application> create_application()
{
	return std::make_unique<SandboxApp>();
}