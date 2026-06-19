#pragma once

namespace Wink
{
	namespace Window { struct Config; }

	class Application
	{
	public:
		virtual ~Application() = default;

		virtual void on_init() {}
		virtual void on_update(double dt) {}
		virtual void on_fixed_update(double dt) {}
		virtual void on_render(double alpha) {}
		virtual void on_shutdown() {}

		[[nodiscard]] virtual Window::Config window_config() const;

		void run();
	};
}