#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/Application.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>
#include <Core/Time.hpp>
#include <WinkEngine/Core/Input.hpp>
#include <WinkEngine/Core/Window.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink
{
	void Application::run()
	{
		Time::init();
		if (!Logger::init()) return;
		if (!Window::init(get_window_config())) return;
		Input::init();
		if (!GFX::init(get_graphics_settings())) return;
		on_init();

		while (Window::is_open())
		{
			ENGINE_ZONE_NAME("Frame");

			Window::poll();
			Input::tick();
			GFX::RES::poll_hot_reloads();
			Time::tick();

			while (Time::should_simulate())
			{
				on_fixed_update(Time::get_fixed_step());
				Time::consume_fixed_step();
			}

			on_update(Time::get_delta());

			pre_render(Time::get_alpha());
			GFX::render();
			post_render();

			Window::swap();

			ENGINE_FRAME_MARK();
		}

		on_shutdown();
		GFX::shutdown();
		Window::shutdown();
		Logger::shutdown();
	}

	Window::Config Application::get_window_config() const { return {}; }
	GFX::Settings Application::get_graphics_settings() const { return {}; }
}