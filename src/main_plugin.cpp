#include "main_plugin.h"

#include <chrono>

namespace vrmapmarkers
{

	// User settings, documented in .ini
	bool g_debug_print = true;

	// Settings
	bool g_left_hand_mode = false;

	void Init()
	{
		ReadConfig(g_ini_path);

		menuchecker::begin();

		auto equip_sink = EventSink<RE::TESEquipEvent>::GetSingleton();
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(equip_sink);
		equip_sink->AddCallback(OnEquipEvent);

		auto SKSE_action_event_sink = EventSink<SKSE::ActionEvent>::GetSingleton();
		SKSE::GetActionEventSource()->AddEventSink(SKSE_action_event_sink);
		SKSE_action_event_sink->AddCallback(OnWeaponDraw);

		auto menu_sink = EventSink<RE::MenuOpenCloseEvent>::GetSingleton();
		RE::UI::GetSingleton()->AddEventSink(menu_sink);
		menu_sink->AddCallback(OnMenuOpenClose);
	}

	void OnGameLoad() { _DEBUGLOG("Load Game: reset state"); }

	void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn)
	{
		if (!evn->opening && std::strcmp(evn->menuName.data(), "Journal Menu") == 0)
		{
			ReadConfig(g_ini_path);

			UpdateMapMarkers();
		}
	}

	void OnWeaponDraw(const SKSE::ActionEvent* event)
	{
		if (event->actor == RE::PlayerCharacter::GetSingleton()->AsReference())
		{
			switch (event->type.get())
			{
			case SKSE::ActionEvent::Type::kBeginSheathe:
			case SKSE::ActionEvent::Type::kEndSheathe:
			case SKSE::ActionEvent::Type::kBeginDraw:
			case SKSE::ActionEvent::Type::kEndDraw:
				UpdateMapMarkers();
			default:
				break;
			}
		}
	}

	void OnEquipEvent(const RE::TESEquipEvent* event)
	{
		if (event->actor &&
			event->actor.get() == RE::PlayerCharacter::GetSingleton()->AsReference())
		{
			if (auto item = RE::TESForm::LookupByID(event->baseObject); item->IsWeapon())
			{
				UpdateMapMarkers();
			}
		}
	}

	void UpdateMapMarkers()
	{
		if (auto map = GetActiveMap(); map != MapType::kNone)
		{
			auto coords = GetTrackedRefLocations();

			for (auto& obj : coords) { AddMarker(obj, map); }
		}
	}

	MapType GetActiveMap() { return MapType::kNone; }

	std::vector<RE::NiPoint3> GetTrackedRefLocations()
	{
		std::vector<RE::NiPoint3> result;

		for (auto& instance : RE::PlayerCharacter::GetSingleton()->objectives)
		{
			auto objective = instance.Objective;
			if (objective->ownerQuest->IsActive() &&
				objective->state == RE::QUEST_OBJECTIVE_STATE::kDisplayed)
			{
				if (auto ref = GetQuestTarget(objective)) { result.push_back(ref->GetPosition()); }
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

	void AddMarker(RE::NiPoint3 a_world, MapType a_map)
	{
		// Check if marker location is inside currently equipped map

		// Get icon transform relative to hand

		// Add icon
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
