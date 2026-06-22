#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/Input.hpp>
#include <WinkEngine/Core/EngineEvents.hpp>

namespace Wink::Input
{
	namespace
	{
		constexpr i32 KEY_COUNT = 512;
		constexpr i32 MOUSE_BTN_COUNT = 8;

		std::array<bool, KEY_COUNT> gKeys = {};
		std::array<bool, KEY_COUNT> gKeysPrev = {};
		std::array<bool, MOUSE_BTN_COUNT> gMouseBtns = {};
		std::array<bool, MOUSE_BTN_COUNT> gMouseBtnsPrev = {};

		double gMouseX = 0.0, gMouseY = 0.0;
		double gMouseDX = 0.0, gMouseDY = 0.0;
		double gScrollX = 0.0, gScrollY = 0.0;

		double gScrollXPending = 0.0, gScrollYPending = 0.0;
	} // anonymous namespace

	void tick()
	{
		gKeysPrev = gKeys;
		gMouseBtnsPrev = gMouseBtns;
		gMouseDX = 0.0;
		gMouseDY = 0.0;
		gScrollX = gScrollXPending;
		gScrollY = gScrollYPending;
		gScrollXPending = 0.0;
		gScrollYPending = 0.0;
	}

    void init()
    {
        subscribe([](const KeyPressEvent& e) {
            if (e.key >= 0 && e.key < KEY_COUNT)
                gKeys[e.key] = true;
            });

        subscribe([](const KeyReleaseEvent& e) {
            if (e.key >= 0 && e.key < KEY_COUNT)
                gKeys[e.key] = false;
            });

        subscribe([](const MouseButtonPressEvent& e) {
            if (e.button >= 0 && e.button < MOUSE_BTN_COUNT)
                gMouseBtns[e.button] = true;
            });

        subscribe([](const MouseButtonReleaseEvent& e) {
            if (e.button >= 0 && e.button < MOUSE_BTN_COUNT)
                gMouseBtns[e.button] = false;
            });

        subscribe([](const MouseMoveEvent& e) {
            gMouseX = e.posX;
            gMouseY = e.posY;
            gMouseDX += e.deltaX;
            gMouseDY += e.deltaY;
            });

        subscribe([](const MouseScrollEvent& e) {
            gScrollXPending += e.offsetX;
            gScrollYPending += e.offsetY;
            });
    }

    bool is_key_down(Code key)
    {
        return (key >= 0 && key < KEY_COUNT) && gKeys[key];
    }

    bool is_key_up(Code key)
    {
        return !is_key_down(key);
    }

    bool is_key_pressed(Code key)
    {
        return (key >= 0 && key < KEY_COUNT) &&
            gKeys[key] && !gKeysPrev[key];
    }

    bool is_key_released(Code key)
    {
        return (key >= 0 && key < KEY_COUNT) &&
            !gKeys[key] && gKeysPrev[key];
    }

    bool is_mouse_button_down(Code button)
    {
        return (button >= 0 && button < MOUSE_BTN_COUNT) && 
            gMouseBtns[button];
    }

    bool is_mouse_button_pressed(Code button)
    {
        return (button >= 0 && button < MOUSE_BTN_COUNT)
            && gMouseBtns[button] && !gMouseBtnsPrev[button];
    }

    bool is_mouse_button_released(Code button)
    {
        return (button >= 0 && button < MOUSE_BTN_COUNT)
            && !gMouseBtns[button] && gMouseBtnsPrev[button];
    }

    double get_mouse_x() { return gMouseX; }
    double get_mouse_y() { return gMouseY; }
    double get_mouse_dx() { return gMouseDX; }
    double get_mouse_dy() { return gMouseDY; }

    double get_scroll_x() { return gScrollX; }
    double get_scroll_y() { return gScrollY; }
}