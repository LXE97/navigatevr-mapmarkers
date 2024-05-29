#pragma once
#include "art_addon.h"
#include "helper_game.h"
#include "helper_math.h"

namespace mapmarker
{
    class MapIcon;

    extern RE::TESFaction* g_stormcloak_faction;
    extern RE::TESFaction* g_dawnguard_faction;
    extern int             g_mod_index;

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

    class MapIcon
    {
    public:
        MapIcon(RE::QUEST_DATA::Type a_type, bool isLeft, RE::NiTransform& a_transform,
            bool a_global, RE::NiPoint2 a_overlap_percent);

        void Update(RE::NiTransform& a_hand_xform);

        RE::TESQuestTarget* target = nullptr;
        HeldMap*            owner = nullptr;

    private:
        static constexpr const char* icon_path = "NavigateVRmarkers/mapmarker.nif";
        static constexpr int         n_border = 2;

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
        RE::NiPoint2           edge_overlap;
    };

    class Manager
    {
    public:
        struct Settings
        {
            bool  use_symbols = true;
            int   selected_border = 0;
            float border_scale = 1.5;
            float symbol_scale = 1.0;
            float regional_scale = 1.5;
            bool  show_custom = true;
            bool  show_player = false;
        };

        enum class State
        {
            kInactive,
            kWaiting,
            kInitialized
        };

        static Manager* GetSingleton()
        {
            static Manager singleton;
            return &singleton;
        }

        const Settings& GetSettings() const { return settings; }
        Settings&       SetSettings() { return settings; }

        State GetState() const { return state; }

        bool FindActiveMap();

        void FindActiveObjectives();

        void ProcessCompassMarker(RE::TESQuestTarget* a_target, RE::NiPoint2 a_pos);

        void AddMarker(RE::NiPoint2 a_world_pos, RE::QUEST_DATA::Type a_type);

        void Refresh();

    private:
        Manager() = default;
        ~Manager() = default;
        Manager(const Manager&) = delete;
        Manager(Manager&&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager& operator=(Manager&&) = delete;

        Settings                              settings;
        State                                 state = State::kInactive;
        std::vector<std::unique_ptr<MapIcon>> icon_addons;
        std::vector<RE::BGSQuestObjective*>   active_objectives;
        std::vector<uintptr_t>      seen_targets;
        const HeldMap*                        active_map = nullptr;
        std::unique_ptr<MapIcon>              custom_marker;
        std::unique_ptr<MapIcon>              player_marker;
    };

    bool TestPointBox2D(RE::NiPoint2 a_point, RE::NiPoint2 bottom_left, RE::NiPoint2 top_right);

    RE::NiPoint2 WorldToMap(RE::NiPoint2 a_world_pos, const HeldMap* a_map);

    RE::NiTransform MapToHand(RE::NiPoint2 a_coords, bool isLeft);

    bool IsSkyrim(const HeldMap* a_map);

    bool IsSolstheim(const HeldMap* a_map);

}