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

	enum HoldLocations
	{
		Tamriel = 0x130FF,
		Haafingar = 0x16770,
		Eastmarch = 0x1676A,
		Whiterun = 0x16772,
		Rift = 0x1676C,
		Reach = 0x16769,
		Falkreath = 0x1676F,
		Hjaalmarch = 0x1676E,
		Pale = 0x1676D,
		Winterhold = 0x1676B,
		Solstheim = 0x16E2A,
	};

	struct MapCalibration
	{  // texture offset
		RE::NiPoint2 local_bottom_left;
		// world coordinates mapping
		RE::NiPoint2 world_bottom_left;
		RE::NiPoint2 world_top_right;
		RE::NiPoint2 xy_scale;
		float angle;
	};

	struct HeldMap
	{
		RE::FormID     armor_form;
		RE::FormID     location_form;
		bool           isLeft;
		MapCalibration data;
	};

	constexpr const char* g_ini_path = "SKSE/Plugins/vrmapmarkers.ini";
	constexpr const char* g_plugin_name = "Navigate";

	extern bool g_left_hand_mode;
	extern bool g_debug_print;

	void Init();

	void OnGameLoad();

	void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn);

	void OnEquipEvent(const RE::TESEquipEvent* event);

	void UpdateMapMarkers();

	HeldMap* GetActiveMap();

	std::vector<RE::TESObjectREFR*> GetTrackedRefs();

	RE::TESObjectREFR* GetQuestTarget(RE::BGSQuestObjective* a_obj);

	void AddMarker(RE::TESObjectREFR* a_objref, HeldMap* a_map);

	void ClearMarkers();

	RE::BGSLocation* GetRootLocation(RE::BGSLocation* a_location);

	RE::NiPoint2 GetMarkerPosition(RE::TESObjectREFR* a_objref);

	RE::TESObjectREFR* GetMapMarker(RE::BGSLocation* a_loc);

	bool TestPointBox2D(RE::NiPoint2 a_point, RE::NiPoint2 bottom_left, RE::NiPoint2 top_right);

	RE::NiTransform WorldToMap(RE::NiPoint2 a_world_pos, HeldMap* a_map);

	/* returns: true if config file changed */
	bool ReadConfig(const char* a_ini_path);
}
