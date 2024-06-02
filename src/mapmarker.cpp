#include "mapmarker.h"

#include "main_plugin.h"
#include "mapmarker_resources.h"

namespace mapmarker
{
    RE::TESFaction* g_dawnguard_faction = nullptr;
    RE::TESFaction* g_stormcloak_faction = nullptr;

    const RE::TESFile* g_base_plugin = nullptr;

    MapIcon::MapIcon(RE::QUEST_DATA::Type a_type, bool isLeft, RE::NiTransform& a_transform,
        bool a_global, RE::NiPoint2 a_overlap_percent)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        type = GetIconType(a_type);
        global = a_global;
        edge_overlap = a_overlap_percent;
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

            if (!global)
            {
                model->Get3D()->local.scale *=
                    settings::Manager::GetSingleton()->Get("fRegionalScale");
            }

            // Select symbol and border
            if (settings::Manager::GetSingleton()->Get("bUseSymbols") && symbol)
            {
                int x, y;
                helper::Arrayize(type, 4, 4, x, y);
                helper::SetUvUnique(symbol, (float)x / 4, (float)y / 4);
                symbol->local.scale = settings::Manager::GetSingleton()->Get("fSymbolScale");
            }

            if (border)
            {
                int x, y;
                helper::Arrayize(settings::Manager::GetSingleton()->Get("iBorderStyle"), n_border,
                    n_border, x, y);

                // offset icon due to map edge
                constexpr float maximum_border_offset = 0.2;

                // translation is opposite sign of uv offset
                border->local.translate.x -= edge_overlap.x;
                border->local.translate.y -= edge_overlap.y;

                // select icon from atlas
                edge_overlap *= maximum_border_offset;
                edge_overlap.x += (float)x * 0.6;
                edge_overlap.y += (float)y * 0.6;

                helper::SetUvUnique(border, edge_overlap.x, edge_overlap.y);

                border->parent->local.scale =
                    settings::Manager::GetSingleton()->Get("fBorderScale");
            }
        }
        else { SKSE::log::error("Map marker creation failed"); }
    }

    void Manager::AddMarker(RE::NiPoint2 a_world_pos, RE::QUEST_DATA::Type a_type)
    {
        if (active_map)
        {
            auto icon_coords = WorldToMap(a_world_pos, active_map);

            _DEBUGLOG("  local map coords: {} {}", icon_coords.x, icon_coords.y);

            // Don't add markers that would be off the map
            if (TestPointBox2D(
                    { icon_coords.x, icon_coords.y }, { 0, 0 }, { kMapWidth, kMapHeight }))
            {
                auto icon_transform = MapToHand(icon_coords, active_map->isLeft);

                // check if border needs to be clipped
                float radius = settings::Manager::GetSingleton()->Get("fBorderScale");
                float x_overlap = 0.f;
                float y_overlap = 0.f;
                if (!IsSkyrim(active_map))
                {
                    radius *= settings::Manager::GetSingleton()->Get("fRegionalScale");
                }

                if (auto right_overlap = icon_coords.x + radius - kMapWidth; right_overlap > 0)
                {
                    x_overlap = -right_overlap / radius;
                }
                else if (auto left_overlap = icon_coords.x - radius; left_overlap < 0)
                {
                    x_overlap = -left_overlap / radius;
                }
                if (auto top_overlap = icon_coords.y + radius - kMapHeight; top_overlap > 0)
                {
                    y_overlap = top_overlap / radius;
                }
                else if (auto bottom_overlap = icon_coords.y - radius; bottom_overlap < 0)
                {
                    y_overlap = bottom_overlap / radius;
                }

                icon_addons.emplace_back(std::make_unique<MapIcon>(a_type, active_map->isLeft,
                    icon_transform, IsSkyrim(active_map), RE::NiPoint2(x_overlap, y_overlap)));

                _DEBUGLOG(" Marker added with local position: {} {} {}",
                    VECTOR(icon_transform.translate));
            }
        }
    }

    bool Manager::FindActiveMap()
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
        if (g_base_plugin->IsFormInMod(mapform))
        {
            if (auto it = std::find_if(g_map_lookup.begin(), g_map_lookup.end(),
                    [mapform](auto& m) { return (m.armor_form & 0xFFF) == (mapform & 0xFFF); });
                it != g_map_lookup.end())
            {
                active_map = &(*it);
                return true;
            }
        }
        active_map = nullptr;
        return false;
    }

    void Manager::Refresh()
    {
        state = State::kInactive;
        active_objectives.clear();
        seen_targets.clear();
        icon_addons.clear();
        custom_marker.reset();
        player_marker.reset();

        if (FindActiveMap())
        {
            FindActiveObjectives();
            state = State::kWaiting;

            if (settings::Manager::GetSingleton()->Get("iShowPlayer"))
            {
                auto pos = RE::PlayerCharacter::GetSingleton()->GetPosition();
                AddMarker({ pos.x, pos.y }, RE::QUEST_DATA::Type::kNone);
            }

            if (settings::Manager::GetSingleton()->Get("iShowCustom"))
            {
                if (auto handle = RE::PlayerCharacter::GetSingleton()->playerMapMarker)
                {
                    if (auto ref = handle.get())
                    {
                        auto pos = ref.get()->GetPosition();
                        AddMarker({ pos.x, pos.y }, RE::QUEST_DATA::Type::kNone);
                    }
                }
            }
        }
    }

    void Manager::ProcessCompassMarker(RE::TESQuestTarget* a_target, RE::NiPoint2 a_pos)
    {
        // Stop if we've already seen this quest target
        for (auto p : seen_targets)
        {
            if (p == (uintptr_t)a_target)
            {
                state = State::kInitialized;
                return;
            }
        }
        seen_targets.push_back((uintptr_t)a_target);

        for (auto obj : active_objectives)
        {
            for (uint32_t i = 0; i < obj->numTargets; ++i)
            {
                if (obj->targets[i] == a_target)
                {
                    _DEBUGLOG("Adding quest marker for {}", obj->ownerQuest->GetName());

                    auto t = obj->ownerQuest->GetType();

                    SKSE::GetTaskInterface()->AddTask(
                        [a_pos, t]() { Manager::GetSingleton()->AddMarker(a_pos, t); });

                    return;
                }
            }
        }
    }

    void Manager::FindActiveObjectives()
    {
        for (auto& instance : RE::PlayerCharacter::GetSingleton()->objectives)
        {
            auto objective = instance.Objective;
            if (objective->ownerQuest->IsActive() &&
                objective->state == RE::QUEST_OBJECTIVE_STATE::kDisplayed)
            {
                active_objectives.push_back(objective);
            }
        }
    }

    bool Manager::IsMap(RE::FormID a_form) const
    {
        // check if form's mod index matches
        if (g_base_plugin->IsFormInMod(a_form))
        {
            // check if lower form is in list of maps.
            if (auto it = std::find_if(g_map_lookup.begin(), g_map_lookup.end(),
                    [a_form](auto& m) { return (m.armor_form & 0xFFF) == (a_form & 0xFFF); });
                it != g_map_lookup.end())
            {
                return true;
            }
        }
        return false;
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
            result.translate = lpos + lrot * RE::NiPoint3(a_coords.x, a_coords.y, 0.05f);
            result.rotate = lrot;
        }
        else
        {
            result.translate = rpos + rrot * RE::NiPoint3(a_coords.x - 39.5, a_coords.y, 0.05f);
            result.rotate = rrot;
        }

        return result;
    };

    bool IsSkyrim(const HeldMap* a_map) { return a_map->location_form == HoldLocations::Tamriel; }

    bool IsSolstheim(const HeldMap* a_map)
    {
        return a_map->location_form == HoldLocations::Solstheim;
    }
}