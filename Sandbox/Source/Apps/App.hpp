#include <WinkEngine/Core.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
		Logger::info("Hello World!");
	}

	void on_update(double dt) override
	{
		Logger::info("DeltaTime: '{}'", dt);
	}

	void on_fixed_update(double dt) override
	{
		Logger::info("Fixed DeltaTime: '{}'", dt);
	}

	void on_render(double alpha) override
	{
		Logger::info("Alpha: '{}'", alpha);
	}

	void on_shutdown() override
	{
		Logger::info("Shutting down");
	}
};