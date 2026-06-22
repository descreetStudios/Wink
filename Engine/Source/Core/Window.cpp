#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/Window.hpp>
#include <WinkEngine/Core/EngineEvents.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <GLFW/glfw3.h>

namespace Wink::Window
{
	namespace
	{
		State gState = State();
		GLFWwindow* gWindow = nullptr;

		GLFWmonitor* get_current_monitor()
		{
			const i32 winX = gState.posX;
			const i32 winY = gState.posY;
			const i32 winW = gState.width;
			const i32 winH = gState.height;

			i32 monitorCount = 0;
			GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
			if (!monitors || monitorCount == 0)
				return glfwGetPrimaryMonitor();

			GLFWmonitor* bestMonitor = glfwGetPrimaryMonitor();
			i32 bestOverlap = -1;

			for (i32 i = 0; i < monitorCount; ++i)
			{
				i32 monX, monY;
				glfwGetMonitorPos(monitors[i], &monX, &monY);

				const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
				if (!mode)
					continue;

				const i32 monW = mode->width;
				const i32 monH = mode->height;

				const i32 overlapX = std::max(0, std::min(winX + winW, monX + monW) - std::max(winX, monX));
				const i32 overlapY = std::max(0, std::min(winY + winH, monY + monH) - std::max(winY, monY));
				const i32 overlap = overlapX * overlapY;

				if (overlap > bestOverlap)
				{
					bestOverlap = overlap;
					bestMonitor = monitors[i];
				}
			}

			return bestMonitor;
		}

#pragma region Callbacks

		void cb_window_close(GLFWwindow*)
		{
			dispatch(WindowCloseEvent{});
		}

		void cb_window_size(GLFWwindow*, i32 width, i32 height)
		{
			if (width <= 0 || height <= 0) return; // minimized
			if (!gState.fullscreen)
			{
				gState.width = static_cast<u32>(width);
				gState.height = static_cast<u32>(height);
			}
			dispatch(WindowResizeEvent{ 
				static_cast<u32>(width), static_cast<u32>(height) });
		}

		void cb_window_pos(GLFWwindow*, i32 posX, i32 posY)
		{
			if (!gState.fullscreen)
			{
				gState.posX = posX;
				gState.posY = posY;
			}
			dispatch(WindowMoveEvent{ posX, posY });
		}

		void cb_window_focus(GLFWwindow*, i32 focused)
		{
			if (focused)
				dispatch(WindowFocusEvent{});
			else
				dispatch(WindowBlurEvent{});
		}

		void cb_window_maximize(GLFWwindow*, i32 maximized)
		{
			gState.maximized = static_cast<bool>(maximized);
			if (maximized)
				dispatch(WindowMaximizeEvent{});
			else
				dispatch(WindowRestoreEvent{});
		}

		void cb_key(GLFWwindow*, i32 key,
			i32 scancode, i32 action, i32 mods)
		{
			switch (action)
			{
			case GLFW_PRESS:
				dispatch(KeyPressEvent{ key, scancode, mods });
				break;
			case GLFW_RELEASE:
				dispatch(KeyReleaseEvent{ key, scancode, mods });
				break;
			case GLFW_REPEAT:
				dispatch(KeyRepeatEvent{ key, scancode, mods });
				break;
			default:
				break;
			}
		}

		void cb_cursor_pos(GLFWwindow*, double posX, double posY)
		{
			static double lastX = posX;
			static double lastY = posY;

			double deltaX = posX - lastX;
			double deltaY = posY - lastY;

			lastX = posX;
			lastY = posY;

			dispatch(MouseMoveEvent{ posX, posY, deltaX, deltaY });
		}

		void cb_cursor_enter(GLFWwindow*, i32 entered)
		{
			if (entered)
				dispatch(MouseEnterEvent{});
			else
				dispatch(MouseLeaveEvent{});
		}

		void cb_mouse_button(GLFWwindow*, i32 button, i32 action, i32 mods)
		{
			if (action == GLFW_PRESS)
				dispatch(MouseButtonPressEvent{ button, mods });
			else if (action == GLFW_RELEASE)
				dispatch(MouseButtonReleaseEvent{ button, mods });
		}

		void cb_scroll(GLFWwindow*, double offsetX, double offsetY)
		{
			dispatch(MouseScrollEvent{ offsetX, offsetY });
		}

		void register_callbacks()
		{
			glfwSetWindowCloseCallback(gWindow, cb_window_close);
			glfwSetWindowSizeCallback(gWindow, cb_window_size);
			glfwSetWindowPosCallback(gWindow, cb_window_pos);
			glfwSetWindowFocusCallback(gWindow, cb_window_focus);
			glfwSetWindowMaximizeCallback(gWindow, cb_window_maximize);
			glfwSetKeyCallback(gWindow, cb_key);
			glfwSetCursorPosCallback(gWindow, cb_cursor_pos);
			glfwSetCursorEnterCallback(gWindow, cb_cursor_enter);
			glfwSetMouseButtonCallback(gWindow, cb_mouse_button);
			glfwSetScrollCallback(gWindow, cb_scroll);
		}

#pragma endregion
	} // anonymous namespace

	bool init(const Config& config)
	{
		if (gWindow)
		{
			Logger::Internal::error(
				"Trying to initialize window multiple times");
			return false;
		}

		if (config.width <= 0 ||
			config.height <= 0)
		{
			Logger::Internal::error(
				"Window dimensions are not valid");
			return false;
		}

		if (!glfwInit())
		{
			Logger::Internal::error("Failed to initialize GLFW");
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

		gWindow = glfwCreateWindow(
			config.width, config.height,
			config.title, nullptr, nullptr);

		if (!gWindow)
		{
			Logger::Internal::error(
				"Failed to initialize window");
			return false;
		}

		glfwMakeContextCurrent(gWindow);

		gState = config;

		register_callbacks();

		set_position(config.posX, config.posY);
		if (gState.fullscreen) set_fullscreen(true);
		if (gState.maximized &&
			!gState.fullscreen) set_maximized(true);
		if (gState.vsync) set_vsync(true);
		if (gState.cursorLocked) lock_cursor(true);

		return true;
	}

	void poll()
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to poll on uninitialized window");
			return;
		}

		glfwPollEvents();
	}

	void swap()
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to swap on uninitialized window");
			return;
		}

		glfwSwapBuffers(gWindow);
	}

	void close()
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to close an uninitialized window");
			return;
		}

		glfwSetWindowShouldClose(gWindow, true);
	}

	void shutdown()
	{
		if (gWindow)
			glfwDestroyWindow(gWindow);
		glfwTerminate();
	}

	void set_fullscreen(bool fullscreen)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set fullscreen mode on an uninitialized window");
			return;
		}

		gState.fullscreen = fullscreen;

		if (fullscreen)
		{
			GLFWmonitor* monitor = get_current_monitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(gWindow, monitor, 0, 0,
				mode->width, mode->height, mode->refreshRate);
		}
		else
		{
			glfwSetWindowMonitor(gWindow, NULL,
				gState.posX, gState.posY,
				gState.width, gState.height, 0);
		}

		dispatch(WindowFullscreenEvent{ fullscreen });
	}

	void toggle_fullscreen() { set_fullscreen(!gState.fullscreen); }

	void set_maximized(bool maximized)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set maximized mode on an uninitialized window");
			return;
		}

		gState.maximized = maximized;

		if (maximized)
			glfwMaximizeWindow(gWindow);
		else
		{
			glfwSetWindowMonitor(gWindow, NULL,
				gState.posX, gState.posY,
				gState.width, gState.height, 0);
		}
	}

	void toggle_maximized() { set_maximized(!gState.maximized); }

	void lock_cursor(bool locked)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set cursor mode on an uninitialized window");
			return;
		}

		gState.cursorLocked = locked;
		glfwSetInputMode(gWindow, GLFW_CURSOR,
			locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	void toggle_cursor_lock() { lock_cursor(!gState.cursorLocked); }

	void set_vsync(bool vsync)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set vsync on an uninitialized window");
			return;
		}

		gState.vsync = vsync;
		glfwSwapInterval(vsync);
	}

	void toggle_vsync() { set_vsync(!gState.vsync); }

	void set_title(const std::string& title)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set title on an uninitialized window");
			return;
		}

		gState.title = title.c_str();
		glfwSetWindowTitle(gWindow, title.c_str());
	}

	void set_position(i32 posX, i32 posY)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set position on an uninitialized window");
			return;
		}

		gState.posX = posX;
		gState.posY = posY;
		glfwSetWindowPos(gWindow, posX, posY);
	}

	void set_pos_x(i32 posX)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set position X on an uninitialized window");
			return;
		}

		gState.posX = posX;
		glfwSetWindowPos(gWindow, posX, gState.posY);
	}

	void set_pos_y(i32 posY)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set position Y on an uninitialized window");
			return;
		}

		gState.posY = posY;
		glfwSetWindowPos(gWindow, gState.posX, posY);
	}

	void set_dimensions(u32 width, u32 height)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set dimensions on an uninitialized window");
			return;
		}

		gState.width = width;
		gState.height = height;
		glfwSetWindowSize(gWindow, width, height);
	}

	void set_width(u32 width)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set width on an uninitialized window");
			return;
		}

		gState.width = width;
		glfwSetWindowSize(gWindow, width, gState.height);
	}

	void set_height(u32 height)
	{
		if (!gWindow)
		{
			Logger::Internal::error(
				"Trying to set height on an uninitialized window");
			return;
		}

		gState.height = height;
		glfwSetWindowSize(gWindow, gState.width, height);
	}

	bool is_open()
	{
		if (!gWindow) return false;
		return !glfwWindowShouldClose(gWindow);
	}

	State get_state() noexcept
	{
		if (!gWindow)
		{
			Logger::Internal::warn(
				"Trying to get state from an uninitialized window");
		}
		return gState;
	}
}