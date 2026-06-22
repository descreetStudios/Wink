#pragma once

#include <WinkEngine/Core/InputTokens.hpp>

namespace Wink::Input
{
	void init();
	void tick();

	/* --- Keyboard --- */
	[[nodiscard]] bool is_key_down(Code key);
	[[nodiscard]] bool is_key_up(Code key);
	[[nodiscard]] bool is_key_pressed(Code key);
	[[nodiscard]] bool is_key_released(Code key);
	
	/* --- Mouse Button --- */
	[[nodiscard]] bool is_mouse_button_down(Code button);
	[[nodiscard]] bool is_mouse_button_pressed(Code button);
	[[nodiscard]] bool is_mouse_button_released(Code button);

	/* --- Mouse Position --- */
	[[nodiscard]] double get_mouse_x();
	[[nodiscard]] double get_mouse_y();
	[[nodiscard]] double get_mouse_dx();
	[[nodiscard]] double get_mouse_dy();

	/* --- Scroll --- */
	[[nodiscard]] double get_scroll_x();
	[[nodiscard]] double get_scroll_y();
}