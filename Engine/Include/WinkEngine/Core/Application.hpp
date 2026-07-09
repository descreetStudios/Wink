#pragma once

namespace Wink
{
	namespace Window { struct Config; }
	namespace GFX { struct Settings; }

	class Application
	{
	public:
		virtual ~Application() = default;

		virtual void on_init() {}
		virtual void on_update(double dt) {}
		virtual void on_fixed_update(double dt) {}
		virtual void pre_render(double alpha) {}
		virtual void post_render() {}
		virtual void on_shutdown() {}

		[[nodiscard]] virtual Window::Config get_window_config() const;
		[[nodiscard]] virtual GFX::Settings get_graphics_settings() const;

		void run();
	};
}