#include "mapmarker.h"

#include "main_plugin.h"
#include "mapmarker_resources.h"

#include <cstdlib>

namespace mapmarker
{
    const auto       kRightMapSlot = RE::BGSBipedObjectForm::BipedObjectSlot::kModMouth;
    const auto       kLeftMapSlot = RE::BGSBipedObjectForm::BipedObjectSlot::kModNeck;
    const RE::FormID kMarkerID = 0x34;
    const float      kMapWidth = 41.4f;
    const float      kMapHeight = 28.8f;
    RE::TESFaction*  g_dawnguard_faction = nullptr;
    RE::TESFaction*  g_stormcloak_faction = nullptr;

    // User Settings
    bool  g_use_symbols = true;
    int   selected_border = 0;
    float g_border_scale = 1.f;
    float g_symbol_scale = 1.f;
    bool  g_use_local_scale = true;
    bool  g_show_playermarker = false;
    bool  g_rotate_border = true;

    // State
    std::vector<std::unique_ptr<mapmarker::MapIcon>> g_icon_addons;
    int                                              g_mod_index = 0;

    MapIcon::MapIcon(RE::QUEST_DATA::Type a_type, bool isLeft, RE::NiTransform& a_transform, bool a_global)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        type = GetIconType(a_type);
        global = a_global;
        model = art_addon::ArtAddon::Make(icon_path, pc,
            pc->Get3D(false)->GetObjectByName(isLeft ? "NPC L Hand [LHnd]" : "NPC R Hand [RHnd]"),
            a_transform, std::bind(&MapIcon::OnCreation, this));
    }

    void MapIcon::OnCreation()
    {
        _DEBUGLOG("marker creation {}", (void*)this);
        if (model && model->Get3D())
        {
            auto symbol = model->Get3D()->GetObjectByName("Symbol");
            auto border = model->Get3D()->GetObjectByName("Border");

            if (g_use_local_scale && global) { model->Get3D()->local.scale *= regional_scale; }

            // Select symbol and border
            if (g_use_symbols && symbol)
            {
                int x, y;
                helper::Arrayize(type, 4, 4, x, y);
                helper::SetUvUnique(symbol, (float)x / 4, (float)y / 4);
                symbol->local.scale = g_symbol_scale;
            }

            if (border)
            {
                int x, y;
                helper::Arrayize(selected_border, 2, 2, x, y);
                helper::SetUvUnique(border, (float)x / 2, (float)y / 2);
                border->parent->local.scale = g_border_scale;

                if (g_rotate_border)
                {
                    auto angle = static_cast<float>(std::rand() % 360);
                    border->parent->local.rotate.SetEulerAnglesXYZ(
                        { 0, 0, helper::deg2rad(angle) });
                }
            }

            // TODO: offset if it's hanging off the map edge
        }
        else { SKSE::log::error("Map marker creation failed"); }
    }

    void UpdateMapMarkers()
    {
        if (auto map = GetActiveMap())
        {
            auto refs = GetTrackedRefs();
            if (refs.empty()) { _DEBUGLOG("No tracked quests found"); }
            else
            {
                // for border rotation to be consistent(>.>) between refreshes:
                std::srand(1);

                for (auto& target : refs)
                {
                    _DEBUGLOG("Adding marker for {}", target.objref->GetName());
                    AddMarker(target, map);
                }
            }
        }
        else { _DEBUGLOG("No map equipped"); }
    }

    const HeldMap* GetActiveMap()
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

        if (g_show_playermarker)
        {
            if (auto custom = RE::PlayerCharacter::GetSingleton()->playerMapMarker)
                if (auto ptr = custom.get())
                {
                    if (auto ref = ptr.get())
                    {
                        result.push_back({ ref, RE::QUEST_DATA::Type::kNone });
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

    void AddMarker(QuestTarget& a_target, const HeldMap* a_map)
    {
        auto objref = a_target.objref;
        bool valid_marker = false;

        // Check if marker location is inside currently equipped map
        if (auto current_loc = objref->GetCurrentLocation(); current_loc)
        {
            _DEBUGLOG(
                "  Quest objective {} location: {}", objref->GetName(), current_loc->GetName());

            if (auto ref_toplevel_location = GetRootLocation(current_loc))
            {
                _DEBUGLOG("    Quest location {} root location: {}", current_loc->GetName(),
                    ref_toplevel_location->GetName());

                // TODO: need to check worldspace for mod refs that have no location assigned


                if (a_map->location_form == (ref_toplevel_location->formID & 0x00FFFFFF) ||
                    (a_map->location_form == HoldLocations::Tamriel &&
                        (ref_toplevel_location->formID & 0x00FFFFFF) != HoldLocations::Solstheim))
                {
                    valid_marker = true;
                }
                else { _DEBUGLOG(" Stop: Quest objective not on active map"); }
            }
        }
        else if (auto base = objref->GetBaseObject(); base && base->GetFormID() == kMarkerID)
        {  // allow player marker to bypass location checks
            valid_marker = g_show_playermarker;
        }

        if (valid_marker)
        {
            auto position = GetMarkerPosition(objref);

            auto icon_coords = WorldToMap(position, a_map);
            _DEBUGLOG("    local map coords: {} {}", icon_coords.x, icon_coords.y);

            // TODO: replace with checking distance from border, for marker masking
            // Check if local map coordinates are within the map model bounds
            if (TestPointBox2D(
                    { icon_coords.x, icon_coords.y }, { 0, 0 }, { kMapWidth, kMapHeight }))
            {
                auto icon_transform = MapToHand(icon_coords, a_map->isLeft);

                g_icon_addons.emplace_back(
                    std::make_unique<MapIcon>(a_target.type, a_map->isLeft, icon_transform, IsSkyrim(a_map)));

                _DEBUGLOG(" Marker added with local position: {} {} {}",
                    VECTOR(icon_transform.translate));
            }
        }
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

    RE::NiPoint2 WorldToMap(RE::NiPoint2 a_world_pos, const HeldMap* a_map)
    {
        RE::NiPoint2 result;

        // Transform world coordinates to held map coordinates
        auto& m1 = a_map->data.upper;
        auto& m2 = a_map->data.lower;

        result.x = m1.x * a_world_pos.x + m1.y * a_world_pos.y + m1.z;
        result.y = m2.x * a_world_pos.x + m2.y * a_world_pos.y + m2.z;

        return result;
    }

    RE::NiTransform MapToHand(RE::NiPoint2 a_coords, bool isLeft)
    {
        // Transform from hand to map "origin node"
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

        // Put the map coordinates into local hand space
        if (isLeft)
        {
            result.translate = lpos + lrot * RE::NiPoint3(a_coords.x, a_coords.y, 0.f);
            result.rotate = lrot;
        }
        else
        {  //TODO: more accurate offset
            result.translate = rpos + rrot * RE::NiPoint3(a_coords.x - 41, a_coords.y, 0.f);
            result.rotate = rrot;
        }

        return result;
    };

    void ClearMarkers()
    {
        _DEBUGLOG("Clearing markers");
        g_icon_addons.clear();
    }

    bool IsSkyrim(const HeldMap* a_map){
        return a_map->location_form == HoldLocations::Tamriel;
    }

    bool IsSolstheim(const HeldMap* a_map){
        return a_map->location_form == HoldLocations::Solstheim;
    }
}