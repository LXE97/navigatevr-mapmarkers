#pragma once
#include "art_addon.h"
#include "helper_game.h"
#include "helper_math.h"

namespace mapmarker
{
    extern RE::TESFaction* g_stormcloak_faction;
    extern RE::TESFaction* g_dawnguard_faction;
    extern int             g_mod_index;

    // User Settings
    extern bool  g_use_symbols;
    extern int   selected_border;
    extern float g_border_scale;
    extern float g_symbol_scale;
    extern float g_regional_scale;
    extern bool  g_show_playermarker;
    extern bool  g_rotate_border;

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
    {
        // Affine transform obtained via the method described here:
        // https://stackoverflow.com/a/2756165
        RE::NiPoint3 upper;
        RE::NiPoint3 lower;
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
        MapIcon(
            RE::QUEST_DATA::Type a_type, bool isLeft, RE::NiTransform& a_transform, bool a_global);

    private:
        static constexpr const char* icon_path = "NavigateVRAddon/mapmarker.nif";

        static int GetIconType(RE::QUEST_DATA::Type a_type)
        {
            if (a_type == RE::QUEST_DATA::Type::kCivilWar &&
                RE::PlayerCharacter::GetSingleton()->IsInFaction(g_stormcloak_faction))
            {
                return 12;
            }
            else if (a_type == RE::QUEST_DATA::Type::kDLC01_Vampire &&
                RE::PlayerCharacter::GetSingleton()->IsInFaction(g_dawnguard_faction))
            {
                return 13;
            }
            return (int)a_type;
        }

        void OnCreation();

        art_addon::ArtAddonPtr model = nullptr;
        int                    type;
        bool                   global = false;
    };

    void UpdateMapMarkers();

    const HeldMap* GetActiveMap();

    std::vector<QuestTarget> GetTrackedRefs();

    RE::TESObjectREFR* GetQuestTarget(RE::BGSQuestObjective* a_obj);

    void AddMarker(QuestTarget& a_target, const HeldMap* a_map);

    void ClearMarkers();

    RE::BGSLocation* GetRootLocation(RE::BGSLocation* a_location);

    RE::NiPoint2 GetMarkerPosition(RE::TESObjectREFR* a_objref);

    RE::TESObjectREFR* GetMapMarker(RE::BGSLocation* a_loc);

    bool TestPointBox2D(RE::NiPoint2 a_point, RE::NiPoint2 bottom_left, RE::NiPoint2 top_right);

    RE::NiPoint2 WorldToMap(RE::NiPoint2 a_world_pos, const HeldMap* a_map);

    RE::NiTransform MapToHand(RE::NiPoint2 a_coords, bool isLeft);

    bool IsSkyrim(const HeldMap* a_map);
    
    bool IsSolstheim(const HeldMap* a_map);

}