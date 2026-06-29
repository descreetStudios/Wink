#pragma once

namespace Wink::Window
{
	struct Config
	{
		const char* title = "Wink Application";
		u32 width = 1280;
		u32 height = 720;
		i32 posX = 320;
		i32 posY = 180;
		bool fullscreen = false;
		bool maximized = false;
		bool vsync = true;
		bool cursorLocked = false;
	};
	using State = Config;

	bool init(const Config& config = {});
	void poll();
	void swap();
	void close();
	void shutdown();

	void set_fullscreen(bool fullscreen);
	void toggle_fullscreen();

	void set_maximized(bool maximized);
	void toggle_maximized();

	void lock_cursor(bool locked);
	void toggle_cursor_lock();

	void set_vsync(bool vsync);
	void toggle_vsync();

	void set_title(const std::string& title);
	void set_position(i32 posX, i32 posY);
	void set_pos_x(i32 posX);
	void set_pos_y(i32 posY);
	void set_dimensions(u32 width, u32 height);
	void set_width(u32 width);
	void set_height(u32 height);

	[[nodiscard]] bool is_open();
	[[nodiscard]] State get_state() noexcept;
}