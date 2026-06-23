#pragma once

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>

/* --- Engine Profiling --- */
#ifdef ENGINE_PROFILE_ENABLED
#define ENGINE_ZONE()					ZoneScoped
#define ENGINE_ZONE_NAME(name)			ZoneScopedN(name)
#define ENGINE_ZONE_COLOR(n, c)			ZoneScopedNC(n, c)
#define ENGINE_FRAME_MARK()				FrameMark
#else
#define ENGINE_ZONE()
#define ENGINE_ZONE_NAME(name)
#define ENGINE_ZONE_COLOR(n, c)
#define ENGINE_FRAME_MARK()
#endif

/* --- App Profiling --- */
#ifdef APP_PROFILE_ENABLED
#define APP_ZONE()                ZoneScoped
#define APP_ZONE_NAME(name)       ZoneScopedN(name)
#define APP_ZONE_COLOR(n, c)      ZoneScopedNC(n, c)
#define APP_FRAME_MARK()          FrameMarkNamed("App Frame")
#else
#define APP_ZONE()
#define APP_ZONE_NAME(name)
#define APP_ZONE_COLOR(n, c)
#define APP_FRAME_MARK()
#endif

#define PROFILE_SET_THREAD_NAME(name)	tracy::SetThreadName(name)
#define PROFILE_PLOT(name, val)			TracyPlot(name, val)
#define PROFILE_MESSAGE(txt, size)		TracyMessage(txt, size)

#else // TRACY_ENABLE

#define ENGINE_ZONE()
#define ENGINE_ZONE_NAME(name)
#define ENGINE_ZONE_COLOR(n, c)
#define ENGINE_FRAME_MARK()

#define APP_ZONE()
#define APP_ZONE_NAME(name)
#define APP_ZONE_COLOR(n, c)
#define APP_FRAME_MARK()

#define PROFILE_SET_THREAD_NAME(name)
#define PROFILE_PLOT(name, val)
#define PROFILE_MESSAGE(txt, size)

#endif