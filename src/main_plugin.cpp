#include "main_plugin.h"

#include <chrono>

namespace vrmapmarkers
{
	const auto kRightMapSlot = RE::BGSBipedObjectForm::BipedObjectSlot::kModMouth;
	const auto kLeftMapSlot = RE::BGSBipedObjectForm::BipedObjectSlot::kModNeck;

	const RE::NiTransform right_offset;
	const RE::NiTransform left_offset;

	// User settings, documented in .ini
	bool g_debug_print = true;

	// Settings
	bool g_left_hand_mode = false;

	// State
	std::vector<RE::ModelReferenceEffect*> g_icon_addons;
	int                                    g_mod_index = 0;

	// Resources
	std::vector<HeldMap> g_map_lookup = {
		{ 0xE5FF, Tamriel, true,
			{ { 1123123, 212312 }, { 0.4222, 0.6222 }, { 0.1233, 0.9123123 }, { 0.111, 0.999 } } },
		{ 0XE600, Tamriel, false },
		{ 0X171E3, Solstheim, true },
		{ 0X171E4, Solstheim, false },
		{ 0X3A353, Tamriel, true },
		{ 0X3A354, Tamriel, true },
		{ 0X3A355, Tamriel, true },  // wyrmstooth
		{ 0X3A356, Tamriel, true },  // bruma
		{ 0X3A357, Tamriel, false },
		{ 0X3A358, Tamriel, false },
		{ 0X3A359, Tamriel, false },  // wyrmstooth
		{ 0X3A35A, Tamriel, false },  // bruma
		{ 0X74B02, Tamriel, true },
		{ 0X74B03, Tamriel, false },
		{ 0XA378E, Whiterun, true },
		{ 0XA378F, Whiterun, false },
		{ 0XB2191, Haafingar, true },
		{ 0XB2192, Falkreath, true },
		{ 0XB2193, Rift, true },
		{ 0XB2194, Reach, true },
		{ 0XB2195, Eastmarch, true },
		{ 0XB2196, Pale, true },
		{ 0XB2197, Winterhold, true },
		{ 0XB2198, Hjaalmarch, true },
		{ 0XB2199, Haafingar, false },
		{ 0XB219A, Falkreath, false },
		{ 0XB219B, Rift, false },
		{ 0XB219C, Reach, false },
		{ 0XB219D, Eastmarch, false },
		{ 0XB219E, Pale, false },
		{ 0XB219F, Winterhold, false },
		{ 0XB21A0, Hjaalmarch, false },
	};

	void Init()
	{
		if (auto file = RE::TESDataHandler::GetSingleton()->LookupModByName(g_plugin_name))
		{
			g_mod_index = file->GetPartialIndex();
		}

		ReadConfig(g_ini_path);

		menuchecker::begin();

		auto equip_sink = EventSink<RE::TESEquipEvent>::GetSingleton();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(equip_sink);
		equip_sink->AddCallback(OnEquipEvent);

		auto menu_sink = EventSink<RE::MenuOpenCloseEvent>::GetSingleton();
		RE::UI::GetSingleton()->AddEventSink(menu_sink);
		menu_sink->AddCallback(OnMenuOpenClose);
	}

	void OnGameLoad()
	{
		_DEBUGLOG("Load Game: reset state");
		if (RE::PlayerCharacter::GetSingleton()->GetCurrent3D()) { UpdateMapMarkers(); }
	}

	void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn)
	{
		if (!evn->opening && std::strcmp(evn->menuName.data(), "Journal Menu") == 0)
		{
			ReadConfig(g_ini_path);

			UpdateMapMarkers();
		}
	}

	void OnEquipEvent(const RE::TESEquipEvent* event)
	{
		if (event->actor && event->actor.get() == RE::PlayerCharacter::GetSingleton()->AsReference())
		{
			if (helper::GetFormIndex(event->baseObject) == g_mod_index)
			{
				_DEBUGLOG(
					"player {} {}", event->equipped ? "equipped" : "unequipped", event->baseObject);
				UpdateMapMarkers();
			}
		}
	}

	void UpdateMapMarkers()
	{
		if (auto map = GetActiveMap())
		{
			auto refs = GetTrackedRefs();
			if (refs.empty()) { ClearMarkers(); }
			else
			{
				for (auto& obj : refs) { AddMarker(obj, map); }
			}
		}
		else { ClearMarkers(); }
	}

	HeldMap* GetActiveMap()
	{
		RE::FormID mapform = NULL;
		if (auto right_worn = RE::PlayerCharacter::GetSingleton()->GetWornArmor(kRightMapSlot))
		{
			mapform = right_worn->formID;
		}
		else if (auto left_worn = RE::PlayerCharacter::GetSingleton()->GetWornArmor(kLeftMapSlot))
		{
			mapform = left_worn->formID;
		}
		if (mapform)
		{
			if (auto it = std::find_if(g_map_lookup.begin(), g_map_lookup.end(),
					[mapform](const HeldMap& m) { return m.armor_form == (mapform & 0x00FFFFFF); });
				it != g_map_lookup.end())
			{
				return &(*it);
			}
		}
		return nullptr;
	}

	std::vector<RE::TESObjectREFR*> GetTrackedRefs()
	{
		std::vector<RE::TESObjectREFR*> result;

		for (auto& instance : RE::PlayerCharacter::GetSingleton()->objectives)
		{
			auto objective = instance.Objective;
			if (objective->ownerQuest->IsActive() &&
				objective->state == RE::QUEST_OBJECTIVE_STATE::kDisplayed)
			{
				if (auto ref = GetQuestTarget(objective)) { result.push_back(ref); }
			}
		}

		return result;
	}

	RE::TESObjectREFR* GetQuestTarget(RE::BGSQuestObjective* a_obj)
	{
		if (a_obj->targets)
		{
			if (auto target = *(a_obj->targets))
			{
				auto alias = target->alias;
				auto quest = a_obj->ownerQuest;
				if (auto it = quest->refAliasMap.find(alias); it != quest->refAliasMap.end())
				{
					auto refhandle = it->second;
					if (auto refptr = refhandle.get()) { return refptr.get(); }
				}
			}
		}
		return nullptr;
	}

	void AddMarker(RE::TESObjectREFR* a_objref, HeldMap* a_map)
	{
		// Check if marker location is inside currently equipped map
		if (auto current_loc = a_objref->GetCurrentLocation())
		{
			if (auto ref_toplevel_location = GetRootLocation(current_loc))
			{
				if (a_map->location_form == (ref_toplevel_location->formID & 0x00FFFFFF) ||
					(a_map->location_form == HoldLocations::Tamriel &&
						(ref_toplevel_location->formID & 0x00FFFFFF) != HoldLocations::Solstheim))
				{
					auto position = GetMarkerPosition(a_objref);

					if (TestPointBox2D(
							position, a_map->data.world_bottom_left, a_map->data.world_top_right))
					{
						// TODO: Get icon transform relative to hand
						auto icon_transform = WorldToMap(position, a_map);

						// TODO: Add icon
					}
				}
			}
		}
	}

	void ClearMarkers()
	{
		for (auto& MRE : g_icon_addons) { MRE->lifetime = 0; }
		g_icon_addons.clear();
	}

	RE::BGSLocation* GetRootLocation(RE::BGSLocation* a_location)
	{
		if (a_location)
		{
			if (a_location->parentLoc == nullptr) { return a_location; }
			else if (a_location->parentLoc->parentLoc == nullptr) { return a_location; }
			else { return GetRootLocation(a_location->parentLoc); }
		}
		return nullptr;
	}

	RE::NiPoint2 GetMarkerPosition(RE::TESObjectREFR* a_objref)
	{
		RE::NiPoint2 result;
		if (a_objref)
		{
			if (auto loc = a_objref->GetCurrentLocation())
			{
				if (auto marker = GetMapMarker(loc))
				{
					auto pos3 = marker->GetPosition();
					result = { pos3.x, pos3.y };
				}
				else if (loc->formID == Tamriel || (loc->formID & 0x00ffffff) == Solstheim)
				{
					auto pos3 = a_objref->GetPosition();
					result = { pos3.x, pos3.y };
				}
			}
		}
		return result;
	}

	RE::TESObjectREFR* GetMapMarker(RE::BGSLocation* a_loc)
	{
		if (a_loc && a_loc->formID != Tamriel)
		{
			if (a_loc->worldLocMarker)
			{
				if (auto ref = a_loc->worldLocMarker.get()) { return ref.get(); }
			}
			return GetMapMarker(a_loc->parentLoc);
		}
		return nullptr;
	}

	bool TestPointBox2D(RE::NiPoint2 a_point, RE::NiPoint2 bottom_left, RE::NiPoint2 top_right)
	{
		return (a_point.x > bottom_left.x && a_point.x < top_right.x && a_point.y > bottom_left.y &&
			a_point.y < top_right.y);
	}

	RE::NiTransform WorldToMap(RE::NiPoint2 a_world_pos, HeldMap* a_map)
	{
		RE::NiTransform result;
		return result;
	}

	bool ReadConfig(const char* a_ini_path)
	{
		using namespace std::filesystem;
		static std::filesystem::file_time_type last_read = {};

		auto config_path = helper::GetGamePath() / a_ini_path;

		if (auto setting = RE::GetINISetting("bLeftHandedMode:VRInput"))
		{
			g_left_hand_mode = setting->GetBool();
		}

		SKSE::log::info("bLeftHandedMode: {}\n ", g_left_hand_mode);

		try
		{
			auto last_write = last_write_time(config_path);

			if (last_write > last_read)
			{
				std::ifstream config(config_path);
				if (config.is_open())
				{
					g_debug_print = helper::ReadIntFromIni(config, "bDebug");

					config.close();
					last_read = last_write_time(config_path);
					return true;
				}
				else
				{
					SKSE::log::error("error opening ini");
					last_read = file_time_type{};
				}
			}
			else { _DEBUGLOG("ini not read (no changes)"); }
		} catch (const filesystem_error&)
		{
			SKSE::log::error("ini not found, using defaults");
			last_read = file_time_type{};
		}
		return false;
	}
}
