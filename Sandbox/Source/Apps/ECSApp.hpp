#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/ECS.hpp>

#include <tracy/Tracy.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
#define TRACY_ENABLE 1

		ECS::Scene scene("PerfTest");

		const size_t count = 1'000'000;
		run_ecs_performance_test(scene, count);

		Logger::info("Test completed!");
	}

private:
	void run_ecs_performance_test(
		ECS::Scene& scene, size_t count)
	{
		ZoneScoped;

		std::vector<ECS::EntityID> entities;
		entities.reserve(count);

		{
			ZoneScopedN("Spawn Entities");

			for (size_t i = 0; i < count; ++i)
			{
				auto e = scene.spawn();
				entities.push_back(e);
			}
		}

		{
			ZoneScopedN("Destroy Entities");

			for (auto& e : entities)
				scene.destroy(e);
		}
	}
};