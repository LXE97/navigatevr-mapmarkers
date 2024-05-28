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

    constexpr const char* g_ini_path = "SKSE/Plugins/vrmapmarkers.ini";
    constexpr const char* g_plugin_name = "Navigate VR - Equipable Dynamic Compass and Maps.esp";

    extern bool g_left_hand_mode;

    void Init();

    void OnGameLoad();

    void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn);

    void OnEquipEvent(const RE::TESEquipEvent* event);

    /* returns: true if config file changed */
    bool ReadConfig(const char* a_ini_path);
}
