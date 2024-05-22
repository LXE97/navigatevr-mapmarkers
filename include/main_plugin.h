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
    extern bool g_debug_print;

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
            auto foo = 6;
        }
        MapIcon(const MapIcon&) = delete;
        MapIcon& operator=(const MapIcon&) = delete;
        MapIcon(MapIcon&& other) noexcept : model(std::move(other.model)), type(other.type) {}
        MapIcon& operator=(MapIcon&& other) noexcept
        {
            if (this != &other)
            {
                model = std::move(other.model);
                type = other.type;
            }
            return *this;
        }

    private:
        static constexpr const char* icon_path = "mapmarker_x.nif";

        static int GetIconType(RE::QUEST_DATA::Type a_type) { return 4; }

        void OnCreation()
        {
            if (model && model->Get3D())
            {
                int x, y;
                helper::Arrayize(type, 4, 4, x, y);
                helper::SetUVCoords(model->Get3D(), x / 4, y / 4);
                _DEBUGLOG("set coords for {}", type);
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
