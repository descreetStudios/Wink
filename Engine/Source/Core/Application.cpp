#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/Application.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <Core/Time.hpp>
#include <WinkEngine/Core/Window.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

namespace Wink
{
	void Application::run()
	{
		Time::init();
		Logger::init();
		if (!Window::init(window_config())) return;
		GFX::init();
		on_init();

		while (Window::is_open())
		{
			Window::poll();

			Time::tick();

			while (Time::should_simulate())
			{
				on_fixed_update(Time::get_fixed_step());
				Time::consume_fixed_step();
			}

			on_update(Time::get_delta());

			on_render(Time::get_alpha());
			GFX::render();

			Window::swap();
		}

		on_shutdown();
		GFX::shutdown();
		Window::shutdown();
		Logger::shutdown();
	}

	Window::Config Application::window_config() const
	{
		return Window::Config();
	}
}