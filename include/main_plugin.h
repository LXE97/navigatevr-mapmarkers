#pragma once

#include "SKSE/Impl/Stubs.h"
#include "menu_checker.h"
#include "mod_event_sink.hpp"
#include "helper_math.h"
#include "helper_game.h"

#define _DEBUGLOG(...) \
	if (vrmapmarkers::g_debug_print) { SKSE::log::trace(__VA_ARGS__); }

namespace vrmapmarkers
{
	constexpr const char* g_ini_path = "SKSE/Plugins/vrmapmarkers.ini";

	extern bool          g_left_hand_mode;
	extern bool          g_debug_print;

	void Init();

	void OnGameLoad();

	/* returns: true if config file changed */
	bool ReadConfig(const char* a_ini_path);
}
