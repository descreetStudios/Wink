#pragma once

#include <eventpp/eventdispatcher.h>

namespace Wink
{
	enum class EventType : u32
	{
		/* --- Window Events --- */
		WindowClose,
		WindowResize,
		WindowMove,
		WindowFocus,
		WindowBlur,
		WindowMaximize,
		WindowRestore,
		WindowFullscreen,

		/* --- Keyboard Events --- */
		KeyPress,
		KeyRelease,
		KeyRepeat,

		/* --- Mouse Events --- */
		MouseMove,
		MouseEnter,
		MouseLeave,
		MouseButtonPress,
		MouseButtonRelease,
		MouseScroll,
	};

	/* --- Window Events --- */
	struct WindowCloseEvent {};

	struct WindowResizeEvent
	{
		u32 width;
		u32 height;
	};

	struct WindowMoveEvent
	{
		i32 posX;
		i32 posY;
	};

	struct WindowFocusEvent {};
	struct WindowBlurEvent {};
	struct WindowMaximizeEvent {};
	struct WindowRestoreEvent {};

	struct WindowFullscreenEvent
	{
		bool fullscreen;
	};

	/* --- Keyboard Events --- */
	struct KeyPressEvent
	{
		i32 key;       // GLFW_KEY_*
		i32 scancode;
		i32 mods;      // GLFW_MOD_*
	};

	struct KeyReleaseEvent
	{
		i32 key;
		i32 scancode;
		i32 mods;
	};

	struct KeyRepeatEvent
	{
		i32 key;
		i32 scancode;
		i32 mods;
	};

	/* --- Mouse Events --- */
	struct MouseMoveEvent
	{
		double posX;
		double posY;
		double deltaX;
		double deltaY;
	};

	struct MouseEnterEvent {};
	struct MouseLeaveEvent {};

	struct MouseButtonPressEvent
	{
		i32 button;
		i32 mods;
	};

	struct MouseButtonReleaseEvent
	{
		i32 button;
		i32 mods;
	};

	struct MouseScrollEvent
	{
		double offsetX;
		double offsetY;
	};

	struct EngineDispatcherPolicy
	{
		using Event = EventType;

		template <typename ...Args>
		static Event getEvent(const EventType& type, Args&&...)
		{
			return type;
		}
	};

	using EngineDispatcher = eventpp::EventDispatcher<
		EventType, void(EventType, const void*),
		EngineDispatcherPolicy>;

	inline EngineDispatcher& get_engine_dispatcher() noexcept
	{
		static EngineDispatcher instance;
		return instance;
	}

	template <typename TEvent, typename TCallback>
	inline auto subscribe(EventType type, TCallback&& cb)
	{
		return get_engine_dispatcher().appendListener(type,
			[cb = std::forward<TCallback>(cb)](EventType, const void* data)
			{
				cb(*static_cast<const TEvent*>(data));
			});
	}

	template <typename TEvent, typename TCallback>
	inline auto subscribe(TCallback&& cb);

	template <typename TEvent>
	inline void dispatch(EventType type, const TEvent& event)
	{
		get_engine_dispatcher().dispatch(
			type, static_cast<const void*>(&event));
	}

#define WINK_EVENT_MAP(EnumVal, Struct)										\
	inline void dispatch(const Struct& e)									\
	{																		\
		get_engine_dispatcher().dispatch(									\
			EventType::EnumVal, static_cast<const void*>(&e));				\
	}																		\
	template <typename TCallback>											\
	inline auto subscribe(TCallback&& cb)									\
	requires std::is_invocable_v<TCallback, const Struct&>					\
	{																		\
		return get_engine_dispatcher().appendListener(EventType::EnumVal,	\
			[cb = std::forward<TCallback>(cb)](EventType, const void* d)	\
			{ cb(*static_cast<const Struct*>(d)); });						\
	}

	WINK_EVENT_MAP(WindowClose, WindowCloseEvent);
	WINK_EVENT_MAP(WindowResize, WindowResizeEvent);
	WINK_EVENT_MAP(WindowMove, WindowMoveEvent);
	WINK_EVENT_MAP(WindowFocus, WindowFocusEvent);
	WINK_EVENT_MAP(WindowBlur, WindowBlurEvent);
	WINK_EVENT_MAP(WindowMaximize, WindowMaximizeEvent);
	WINK_EVENT_MAP(WindowRestore, WindowRestoreEvent);
	WINK_EVENT_MAP(WindowFullscreen, WindowFullscreenEvent);
	WINK_EVENT_MAP(KeyPress, KeyPressEvent);
	WINK_EVENT_MAP(KeyRelease, KeyReleaseEvent);
	WINK_EVENT_MAP(KeyRepeat, KeyRepeatEvent);
	WINK_EVENT_MAP(MouseMove, MouseMoveEvent);
	WINK_EVENT_MAP(MouseEnter, MouseEnterEvent);
	WINK_EVENT_MAP(MouseLeave, MouseLeaveEvent);
	WINK_EVENT_MAP(MouseButtonPress, MouseButtonPressEvent);
	WINK_EVENT_MAP(MouseButtonRelease, MouseButtonReleaseEvent);
	WINK_EVENT_MAP(MouseScroll, MouseScrollEvent);

#undef WINK_EVENT_MAP
}