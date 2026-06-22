#pragma once

#include <WinkEngine/Core.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
		Logger::set_level(Logger::Level::Trace);

		subscribe_to_events();
	}

private:
	void subscribe_to_events()
	{
		using namespace Input;

		/* --- Window --- */
		subscribe([](const WindowCloseEvent&) {
			Logger::info("Window closing");
			});

		subscribe([](const WindowResizeEvent& e) {
			Logger::trace("Resized: '{}x{}'", e.width, e.height);
			});

		subscribe([](const WindowMoveEvent& e) {
			Logger::trace("Moved: '{}x{}'", e.posX, e.posY);
			});

		subscribe([](const WindowFocusEvent&) {
			Logger::info("Window focused");
			});

		subscribe([](const WindowBlurEvent&) {
			Logger::info("Window unfocused");
			});

		subscribe([](const WindowMaximizeEvent&) {
			Logger::info("Window maximized");
			});

		subscribe([](const WindowRestoreEvent&) {
			Logger::info("Window restored");
			});

		subscribe([](const WindowFullscreenEvent& e) {
			Logger::info("Window fullscreen is '{}'", e.fullscreen);
			});

		/* --- Keyboard --- */
		subscribe([](const KeyPressEvent& e) {
			if (e.key == Key::Enter && (e.mods & Mod::Alt))
				Window::toggle_fullscreen();
			});

		subscribe([](const KeyReleaseEvent& e) {
			if (e.key == Key::Escape)
				Window::close();
			});

		subscribe([](const KeyRepeatEvent& e) {
			if (e.key == Key::V)
				Logger::info("V key repeat");
			});

		/* --- Mouse --- */
		subscribe([](const MouseMoveEvent& e) {
			Logger::trace("Mouse moved to '{}x{}'. Delta: '{}x{}'",
				e.posX, e.posY, e.deltaX, e.deltaY);
			});

		subscribe([](const MouseEnterEvent&) {
			Logger::info("Mouse entered");
			});

		subscribe([](const MouseLeaveEvent&) {
			Logger::info("Mouse left");
			});

		subscribe([](const MouseButtonPressEvent& e) {
			if (e.button == MouseButton::Left)
				Logger::info("Clicked");
			});

		subscribe([](const MouseButtonReleaseEvent& e) {
			if (e.button == MouseButton::Left)
				Logger::info("Released");
			});

		subscribe([](const MouseScrollEvent& e) {
			Logger::trace("Scrolled by '{}, {}'", e.offsetX, e.offsetY);
			});
	}
};