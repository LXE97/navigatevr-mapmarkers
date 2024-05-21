#pragma once

#include "SKSE/Impl/Stubs.h"
#include "helper_game.h"
#include "helper_math.h"
#include "menu_checker.h"
#include "mod_event_sink.hpp"

#define _DEBUGLOG(...) \
	if (vrmapmarkers::g_debug_print) { SKSE::log::trace(__VA_ARGS__); }

namespace vrmapmarkers
{
	enum class MapType
	{
		kNone = 0,
	};

	constexpr const char* g_ini_path = "SKSE/Plugins/vrmapmarkers.ini";

	extern bool g_left_hand_mode;
	extern bool g_debug_print;

	void Init();

	void OnGameLoad();

	void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn);

	void OnWeaponDraw(const SKSE::ActionEvent* event);

	void OnEquipEvent(const RE::TESEquipEvent* event);

	void UpdateMapMarkers();

	std::vector<RE::NiPoint3> GetTrackedRefLocations();

	RE::TESObjectREFR* GetQuestTarget(RE::BGSQuestObjective* a_obj);

	void AddMarker(RE::NiPoint3 a_world, MapType a_map);

	MapType GetActiveMap();

	/* returns: true if config file changed */
	bool ReadConfig(const char* a_ini_path);
}
