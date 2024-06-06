#pragma once

#include "Relocation.h"
#include "SKSE/Impl/Stubs.h"
#include "art_addon.h"
#include "helper_game.h"
#include "helper_math.h"
#include "mapmarker.h"
#include "menu_checker.h"
#include "mod_event_sink.hpp"
#include "xbyak/xbyak.h"

#define _DEBUGLOG(...) \
    if (vrmapmarkers::g_debug_print) { SKSE::log::trace(__VA_ARGS__); }

namespace vrmapmarkers
{
    extern bool g_debug_print;

	constexpr const char* g_mod_name = "vrmapmarkers";
	constexpr RE::FormID  g_settings_formlist = 0x801;
    constexpr const char* g_plugin_name = "Navigate VR - Equipable Dynamic Compass and Maps.esp";

    void Init();

    void OnGameLoad();

    void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn);

    void OnEquipEvent(const RE::TESEquipEvent* event);
}
