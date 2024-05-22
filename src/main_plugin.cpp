#include "main_plugin.h"

#include <chrono>

namespace vrmapmarkers
{
    using namespace art_addon;

    const auto kRightMapSlot = RE::BGSBipedObjectForm::BipedObjectSlot::kModMouth;
    const auto kLeftMapSlot = RE::BGSBipedObjectForm::BipedObjectSlot::kModNeck;

    // User settings, documented in .ini
    bool g_debug_print = true;

    // Settings
    bool g_left_hand_mode = false;

    // State
    std::vector<std::unique_ptr<MapIcon>> g_icon_addons;

    // Resources
    int                   g_mod_index = 0;
    RE::TESFaction*       g_dawnguard_faction = nullptr;
    RE::FormID            g_dawnguard_faction_id = 0x14217;
    constexpr const char* g_dawnguard_plugin_name = "Dawnguard.esm";
    RE::TESFaction*       g_stormcloak_faction = nullptr;
    RE::FormID            g_stormcloak_faction_id = 0x2bf9b;

    const MapCalibration tamriel_offsets_L = { { 0, 0 }, { -200000, -150000 }, { 200000, 150000 },
        { 0.0000625, 0.0000625 } };

    const MapCalibration tamriel_offsets_R = { { 0, 0 }, { -200000, -150000 }, { 200000, 150000 },
        { 0.0000625, 0.0000625 } };

    std::vector<HeldMap> g_map_lookup = {
        { 0xE5FF, Tamriel, true, tamriel_offsets_L },
        { 0XE600, Tamriel, false, tamriel_offsets_R },
        { 0X171E3, Solstheim, true },
        { 0X171E4, Solstheim, false },
        { 0X3A353, Tamriel, true, tamriel_offsets_L },
        { 0X3A354, Tamriel, true, tamriel_offsets_L },
        { 0X3A355, Tamriel, true, tamriel_offsets_L },  // wyrmstooth
        { 0X3A356, Tamriel, true, tamriel_offsets_L },  // bruma
        { 0X3A357, Tamriel, false, tamriel_offsets_R },
        { 0X3A358, Tamriel, false, tamriel_offsets_R },
        { 0X3A359, Tamriel, false, tamriel_offsets_R },  // wyrmstooth
        { 0X3A35A, Tamriel, false, tamriel_offsets_R },  // bruma
        { 0X74B02, Tamriel, true, tamriel_offsets_L },
        { 0X74B03, Tamriel, false, tamriel_offsets_R },
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

    void PlayerUpdate()
    {
        static auto mgr = ArtAddonManager::GetSingleton();
        mgr->Update();
    }

    void Init()
    {
        helper::InstallPlayerUpdateHook(PlayerUpdate);

        if (auto file = RE::TESDataHandler::GetSingleton()->LookupModByName(g_plugin_name))
        {
            g_mod_index = file->GetPartialIndex();
        }
        else
        {
            SKSE::log::error(
                "NavigateVR ESP not found, please ensure you enabled the mod and did not rename "
                "the plugin file");
        }

        g_dawnguard_faction =
            helper::GetForm<RE::TESFaction>(g_dawnguard_faction_id, g_dawnguard_plugin_name);

        g_stormcloak_faction = RE::TESForm::LookupByID<RE::TESFaction>(g_stormcloak_faction_id);

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

        g_icon_addons.clear();
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
        if (event->actor &&
            event->actor.get() == RE::PlayerCharacter::GetSingleton()->AsReference())
        {
            if (helper::GetFormIndex(event->baseObject) == g_mod_index)
            {
                _DEBUGLOG("player {} {:x}", event->equipped ? "equipped" : "unequipped",
                    event->baseObject);

                if (event->equipped) { UpdateMapMarkers(); }
                else { ClearMarkers(); }
            }
        }
    }

    void UpdateMapMarkers()
    {
        ClearMarkers();
        if (auto map = GetActiveMap())
        {
            auto refs = GetTrackedRefs();
            if (refs.empty()) { _DEBUGLOG("No tracked quests found"); }
            else
            {
                for (auto& target : refs)
                {
                    _DEBUGLOG("Adding marker for {}", target.objref->GetName());
                    AddMarker(target, map);
                }
            }
        }
        else { _DEBUGLOG("No map equipped"); }
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
        if (helper::GetFormIndex(mapform) == g_mod_index)
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

    std::vector<QuestTarget> GetTrackedRefs()
    {
        std::vector<QuestTarget> result;

        for (auto& instance : RE::PlayerCharacter::GetSingleton()->objectives)
        {
            auto objective = instance.Objective;
            if (objective->ownerQuest->IsActive() &&
                objective->state == RE::QUEST_OBJECTIVE_STATE::kDisplayed)
            {
                if (auto ref = GetQuestTarget(objective))
                {
                    result.push_back({ ref, objective->ownerQuest->GetType() });
                }
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
                _DEBUGLOG("quest type: {}", quest->data.questType.underlying());
                if (auto it = quest->refAliasMap.find(alias); it != quest->refAliasMap.end())
                {
                    auto refhandle = it->second;
                    if (auto refptr = refhandle.get()) { return refptr.get(); }
                }
            }
        }
        return nullptr;
    }

    void AddMarker(QuestTarget& a_target, HeldMap* a_map)
    {
        auto objref = a_target.objref;
        // Check if marker location is inside currently equipped map
        if (auto current_loc = objref->GetCurrentLocation())
        {
            _DEBUGLOG(
                "  Quest objective {} location: {}", objref->GetName(), current_loc->GetName());

            if (auto ref_toplevel_location = GetRootLocation(current_loc))
            {
                _DEBUGLOG("    Quest location {} root location: {}", current_loc->GetName(),
                    ref_toplevel_location->GetName());

                if (a_map->location_form == (ref_toplevel_location->formID & 0x00FFFFFF) ||
                    (a_map->location_form == HoldLocations::Tamriel &&
                        (ref_toplevel_location->formID & 0x00FFFFFF) != HoldLocations::Solstheim))
                {
                    auto position = GetMarkerPosition(objref);

                    if (TestPointBox2D(
                            position, a_map->data.world_bottom_left, a_map->data.world_top_right))
                    {
                        auto icon_transform = WorldToMap(position, a_map);

                        g_icon_addons.emplace_back(std::make_unique<MapIcon>(
                            a_target.type, a_map->isLeft, icon_transform));

                        _DEBUGLOG(" Marker added with local position: {} {} {}",
                            VECTOR(icon_transform.translate));
                    }
                }
                else { _DEBUGLOG(" Stop: Quest objective not on active map"); }
            }
        }
    }

    void ClearMarkers()
    {
        _DEBUGLOG("Clearing markers");
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

                    _DEBUGLOG("  Using MapMarker position {} : {} {}", marker->GetName(), result.x,
                        result.y);
                    return result;
                }
                //else if (loc->formID == Tamriel || (loc->formID & 0x00ffffff) == Solstheim) {}
            }
            auto pos3 = a_objref->GetPosition();
            result = { pos3.x, pos3.y };

            _DEBUGLOG("  Using objref position {} {}", result.x, result.y);
        }
        return result;
    }

    RE::TESObjectREFR* GetMapMarker(RE::BGSLocation* a_loc)
    {
        if (a_loc)
        {  // We only want to get map markers for exterior locations, at most 2 levels from Tamriel
            if (a_loc->parentLoc == nullptr || a_loc->parentLoc->parentLoc == nullptr ||
                a_loc->parentLoc->parentLoc->parentLoc == nullptr)
            {
                if (a_loc->worldLocMarker)
                {
                    if (auto ref = a_loc->worldLocMarker.get()) { return ref.get(); }
                }
            }
            else { return GetMapMarker(a_loc->parentLoc); }
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
        const RE::NiPoint3  lpos = { -1.261147, -4.375540, 9.100316 };
        const RE::NiMatrix3 lrot = {
            { -0.1650175, -0.9329688, -0.3199038 },
            { -0.9815594, 0.1236156, 0.1458094 },
            { -0.0964906, 0.3380657, -0.9361630 },
        };
        const RE::NiPoint3  rpos = { 1.261147, -4.375540, 9.100316 };
        const RE::NiMatrix3 rrot = {
            { -0.1650175, 0.9329688, 0.3199038 },
            { 0.9815594, 0.1236156, 0.1458094 },
            { 0.0964906, 0.3380657, -0.9361630 },
        };

        RE::NiTransform result;

        auto local =
            helper::Rotate2D(a_world_pos - a_map->data.world_bottom_left, -a_map->data.angle);
        local.x *= a_map->data.xy_scale.x;
        local.y *= a_map->data.xy_scale.y;

        if (a_map->isLeft)
        {
            result.translate = lpos + lrot * RE::NiPoint3(local.x, local.y, 0.f);
            result.rotate = lrot;
        }
        else
        {
            result.translate = rpos + rrot * RE::NiPoint3(local.x, local.y, 0.f);
            result.rotate = rrot;
            result.translate.y *= -1.f;
        }

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
