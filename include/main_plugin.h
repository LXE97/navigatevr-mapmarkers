#pragma once

#include "SKSE/Impl/Stubs.h"
#include "art_addon.h"
#include "helper_game.h"
#include "helper_math.h"
#include "menu_checker.h"
#include "mod_event_sink.hpp"

#define _DEBUGLOG(...) \
    if (vrmapmarkers::g_debug_print) { SKSE::log::trace(__VA_ARGS__); }

namespace vrmapmarkers
{
    extern bool            g_debug_print;
    extern RE::TESFaction* g_stormcloak_faction;
    extern RE::TESFaction* g_dawnguard_faction;

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
        float        angle;
    };

    struct HeldMap
    {
        RE::FormID     armor_form;
        RE::FormID     location_form;
        bool           isLeft;
        MapCalibration data;
    };

    struct QuestTarget
    {
        RE::TESObjectREFR*   objref;
        RE::QUEST_DATA::Type type;
    };

    class MapIcon
    {
    public:
        MapIcon(RE::QUEST_DATA::Type a_type, bool isLeft, RE::NiTransform& a_transform)
        {
            auto pc = RE::PlayerCharacter::GetSingleton();
            type = GetIconType(a_type);
            model = art_addon::ArtAddon::Make(icon_path, pc,
                pc->Get3D(false)->GetObjectByName(
                    isLeft ? "NPC L Hand [LHnd]" : "NPC R Hand [RHnd]"),
                a_transform, std::bind(&MapIcon::OnCreation, this));
        }

    private:
        static constexpr const char* icon_path = "mapmarker_x.nif";

        static int GetIconType(RE::QUEST_DATA::Type a_type)
        {
            if (a_type == RE::QUEST_DATA::Type::kCivilWar &&
                RE::PlayerCharacter::GetSingleton()->IsInFaction(g_dawnguard_faction))
            {
                return 12;
            }
            else if (a_type == RE::QUEST_DATA::Type::kDLC01_Vampire &&
                RE::PlayerCharacter::GetSingleton()->IsInFaction(g_stormcloak_faction))
            {
                return 13;
            }
            return (int)a_type;
        }

        void OnCreation()
        {
            _DEBUGLOG("marker creation {}", (void*)this);
            if (model && model->Get3D())
            {
                int x, y;
                helper::Arrayize(type, 4, 4, x, y);
                if (auto shader = helper::GetShaderProperty(model->Get3D(), "Plane"))
                {
                    auto oldmat = shader->material;
                    auto newmat = oldmat->Create();
                    newmat->CopyMembers(oldmat);
                    shader->material = newmat;
                    newmat->IncRef();
                    oldmat->DecRef();

                    newmat->texCoordOffset[0].x = (float)x / 4;
                    newmat->texCoordOffset[0].y = (float)y / 4;
                    newmat->texCoordOffset[1].x = newmat->texCoordOffset[0].x;
                    newmat->texCoordOffset[1].y = newmat->texCoordOffset[0].y;
                    _DEBUGLOG("set coords for {}", type);
                }
                
            }
            else { SKSE::log::error("Map marker creation failed"); }
        }

        art_addon::ArtAddonPtr model = nullptr;
        int                    type;
    };

    constexpr const char* g_ini_path = "SKSE/Plugins/vrmapmarkers.ini";
    constexpr const char* g_plugin_name = "Navigate VR - Equipable Dynamic Compass and Maps.esp";

    extern bool g_left_hand_mode;

    void Init();

    void OnGameLoad();

    void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn);

    void OnEquipEvent(const RE::TESEquipEvent* event);

    void UpdateMapMarkers();

    HeldMap* GetActiveMap();

    std::vector<QuestTarget> GetTrackedRefs();

    RE::TESObjectREFR* GetQuestTarget(RE::BGSQuestObjective* a_obj);

    void AddMarker(QuestTarget& a_target, HeldMap* a_map);

    void ClearMarkers();

    RE::BGSLocation* GetRootLocation(RE::BGSLocation* a_location);

    RE::NiPoint2 GetMarkerPosition(RE::TESObjectREFR* a_objref);

    RE::TESObjectREFR* GetMapMarker(RE::BGSLocation* a_loc);

    bool TestPointBox2D(RE::NiPoint2 a_point, RE::NiPoint2 bottom_left, RE::NiPoint2 top_right);

    RE::NiTransform WorldToMap(RE::NiPoint2 a_world_pos, HeldMap* a_map);

    void InitIcon(int a);

    /* returns: true if config file changed */
    bool ReadConfig(const char* a_ini_path);
}
