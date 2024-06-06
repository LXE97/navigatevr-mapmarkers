#include "mapmarker.h"

#include "main_plugin.h"
#include "mapmarker_resources.h"

namespace mapmarker
{
    RE::TESFaction* g_dawnguard_faction = nullptr;
    RE::TESFaction* g_stormcloak_faction = nullptr;

    const RE::TESFile* g_base_plugin = nullptr;

    MapIcon::MapIcon(MapIcon::IconType a_type, const HeldMap* a_map, RE::NiTransform& a_transform,
        RE::NiPoint2 a_overlap_percent)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        type = a_type;
        owner = a_map;
        edge_overlap = a_overlap_percent;
        model = art_addon::ArtAddon::Make(type > kCustom ? kIconPathAlt : kIconPath, pc,
            pc->Get3D(false)->GetObjectByName(
                a_map->isLeft ? "NPC L Hand [LHnd]" : "NPC R Hand [RHnd]"),
            a_transform, std::bind(&MapIcon::OnCreation, this));
    }

    void MapIcon::OnCreation()
    {
        _DEBUGLOG("marker creation {}", (void*)this);
        if (model && model->Get3D())
        {
            auto symbol = model->Get3D()->GetObjectByName("Symbol");
            auto border = model->Get3D()->GetObjectByName("Border");

            if (settings::Manager::GetSingleton()->Get("fBrightness") > 0.f)
            {
                helper::SetGlowMult(settings::Manager::GetSingleton()->Get("fBrightness"), border);
                helper::SetGlowMult(settings::Manager::GetSingleton()->Get("fBrightness"), symbol);
            }

            if (!IsSkyrim(owner))
            {
                model->Get3D()->local.scale *=
                    settings::Manager::GetSingleton()->Get("fRegionalScale");
            }

            // Select symbol for non-Player, non-Custom markers
            if (type < 14 && settings::Manager::GetSingleton()->Get("iShowSymbol") && symbol)
            {
                int x, y;

                int symbol_option = settings::Manager::GetSingleton()->Get("iShowSymbol");

                if (symbol_option == 1)
                {  // index of "X" marker. option = "only Xes"
                    helper::Arrayize(14, 4, 4, x, y);
                }
                else if (symbol_option == 2)
                {  // "faction only"
                    helper::Arrayize(type, 4, 4, x, y);
                }
                else
                {  // do faction specific or X for non faction types
                    if (type == 0 || type == 6 || type == 8) { helper::Arrayize(14, 4, 4, x, y); }
                    else { helper::Arrayize(type, 4, 4, x, y); }
                }

                helper::SetUvUnique(symbol, (float)x / 4, (float)y / 4);
                symbol->local.scale = settings::Manager::GetSingleton()->Get("fSymbolScale");
            }

            if (border)
            {
                int x, y;

                switch (type)
                {
                case MapIcon::IconType::kPlayerAlt:
                    {
                        auto style = settings::Manager::GetSingleton()->Get("iPlayerStyle");
                        if (style > 3) style -= 4;
                    }
                case MapIcon::IconType::kPlayer:
                    helper::Arrayize(settings::Manager::GetSingleton()->Get("iPlayerStyle"),
                        n_border, n_border, x, y);
                    border->parent->local.scale =
                        settings::Manager::GetSingleton()->Get("fPlayerScale");
                    break;
                case MapIcon::IconType::kCustomAlt:
                    {
                        auto style = settings::Manager::GetSingleton()->Get("iCustomStyle");
                        if (style > 3) style -= 4;
                    }
                case MapIcon::IconType::kCustom:
                    helper::Arrayize(settings::Manager::GetSingleton()->Get("iCustomStyle"),
                        n_border, n_border, x, y);
                    border->parent->local.scale =
                        settings::Manager::GetSingleton()->Get("fBorderScale");
                    break;
                default:
                    if (settings::Manager::GetSingleton()->Get("iBorderStyle"))
                    {
                        helper::Arrayize(settings::Manager::GetSingleton()->Get("iBorderStyle") - 1,
                            n_border, n_border, x, y);
                        border->parent->local.scale =
                            settings::Manager::GetSingleton()->Get("fBorderScale");
                    }
                    else
                    {
                        border->parent->local.scale = 0;
                        // if borders are disabled, we're done
                        return;
                    }
                }

                // offset icon due to map edge
                constexpr float maximum_border_offset = 0.2;

                border->local.translate.x = edge_overlap.x;
                // +V = down
                border->local.translate.y -= edge_overlap.y;

                // select icon from atlas
                edge_overlap *= maximum_border_offset;
                edge_overlap.x += (float)x * 0.6;
                edge_overlap.y += (float)y * 0.6;

                helper::SetUvUnique(border, edge_overlap.x, edge_overlap.y);
            }
        }
        else { SKSE::log::error("Map marker creation failed"); }
    }

    void MapIcon::Update(RE::NiPoint2 a_pos)
    {
        if (model && model->Get3D())
        {
            auto icon_coords = WorldToMap(a_pos, Manager::GetSingleton()->GetActiveMap());

            if (TestPointBox2D(
                    icon_coords, { kMapLowerWidth, kMapLowerHeight }, { kMapWidth, kMapHeight }))
            {
                // using player scale since it's the only one that updates
                auto radius = model->Get3D()->local.scale *
                    settings::Manager::GetSingleton()->Get("fPlayerScale");

                edge_overlap = TestOverlap(icon_coords, radius);
                if (edge_overlap != RE::NiPoint2(0, 0))
                {
                    if (auto border = model->Get3D()->GetObjectByName("Border"))
                    {
                        int x, y;
                        // hardcoding player, should be switching on type but non-player markers never update
                        helper::Arrayize(settings::Manager::GetSingleton()->Get("iPlayerStyle"),
                            n_border, n_border, x, y);

                        // offset icon due to map edge
                        constexpr float maximum_border_offset = 0.2;

                        border->local.translate.x = edge_overlap.x;
                        // y translation is opposite sign of uv offset
                        border->local.translate.y = -edge_overlap.y;

                        // select icon from atlas
                        edge_overlap *= maximum_border_offset;
                        edge_overlap.x += (float)x * 0.6;
                        edge_overlap.y += (float)y * 0.6;

                        helper::SetUVCoords(border, edge_overlap.x, edge_overlap.y);
                    }
                }

                RE::NiUpdateData ctx;
                auto xform = Manager::GetSingleton()->MapToHand(icon_coords, owner->isLeft, false);
                model->Get3D()->local.translate = xform.translate;
                model->Get3D()->local.rotate = xform.rotate;
                model->Get3D()->Update(ctx);
            }
        }
    }

    void Manager::AddMarker(RE::NiPoint2 a_world_pos, MapIcon::IconType a_type)
    {
        if (active_map)
        {
            _DEBUGLOG("Adding marker with world position: {} {}", a_world_pos.x, a_world_pos.y);
            auto icon_coords = WorldToMap(a_world_pos, active_map);

            _DEBUGLOG("  local map coords: {} {}", icon_coords.x, icon_coords.y);

            // Don't add markers that would be off the map
            if (TestPointBox2D(
                    icon_coords, { kMapLowerWidth, kMapLowerHeight }, { kMapWidth, kMapHeight }))
            {
                auto icon_transform = MapToHand(icon_coords, active_map->isLeft);

                // check if border needs to be clipped
                float radius = settings::Manager::GetSingleton()->Get("fBorderScale");
                if (!IsSkyrim(active_map))
                {
                    radius *= settings::Manager::GetSingleton()->Get("fRegionalScale");
                }

                auto overlap = TestOverlap(icon_coords, radius);

                switch (a_type)
                {
                case MapIcon::IconType::kPlayer:
                    if (settings::Manager::GetSingleton()->Get("iPlayerStyle") < 4)
                    {
                        a_type = MapIcon::IconType::kPlayerAlt;
                    }
                    player_marker =
                        std::make_unique<MapIcon>(a_type, active_map, icon_transform, overlap);
                    break;
                case MapIcon::IconType::kCustom:
                    if (settings::Manager::GetSingleton()->Get("iCustomStyle") < 4)
                    {
                        a_type = MapIcon::IconType::kCustomAlt;
                    }
                    custom_marker =
                        std::make_unique<MapIcon>(a_type, active_map, icon_transform, overlap);
                    break;
                default:
                    icon_addons.emplace_back(
                        std::make_unique<MapIcon>(a_type, active_map, icon_transform, overlap));
                }

                _DEBUGLOG("  Marker added with hand transform: {} {} {}",
                    VECTOR(icon_transform.translate));
            }
            else { _DEBUGLOG("  marker outside map limits"); }
        }
    }

    void Manager::UpdatePlayerMarker()
    {
        static int c = 0;
        if (active_map)
        {
            if (player_marker)
            {
                if (++c % 40 == 0)
                {
                    auto pos = RE::PlayerCharacter::GetSingleton()->GetPosition();
                    player_marker->Update({ pos.x, pos.y });
                }
            }
        }
    }

    void Manager::OnPlayerEquip(RE::FormID a_equipitem, bool a_equipped)
    {
        if (IsMap(a_equipitem))
        {
            if (a_equipped) { Refresh(); }
            else { Clear(); }
        }
        else if (a_equipitem == kSpellID && state != State::kInactive)
        {
            if (a_equipped)
            {
                if ((int)settings::Manager::GetSingleton()->Get("iShowPlayer") == 2)
                {
                    DrawPlayerMarker();
                }

                if ((int)settings::Manager::GetSingleton()->Get("iShowCustom") == 2)
                {
                    DrawCustomMarker();
                }
            }
            else
            {
                if ((int)settings::Manager::GetSingleton()->Get("iShowPlayer") == 2)
                {
                    player_marker.reset();
                }

                if ((int)settings::Manager::GetSingleton()->Get("iShowCustom") == 2)
                {
                    custom_marker.reset();
                }
            }
        }
    }

    bool Manager::FindActiveMap()
    {
        RE::FormID mapform = NULL;
        if (auto right_worn = RE::PlayerCharacter::GetSingleton()->GetWornArmor(kRightMapSlot);
            right_worn && (right_worn->formID & 0xffff) != 0xe09d &&
            (right_worn->formID & 0xffff) != 0x0d62)
        {
            mapform = right_worn->formID;
        }
        else if (auto left_worn = RE::PlayerCharacter::GetSingleton()->GetWornArmor(kLeftMapSlot);
                 left_worn && (left_worn->formID & 0xffff) != 0xe09d &&
                 (left_worn->formID & 0xffff) != 0x0d62)
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
                _DEBUGLOG("Equipped map: {:x}", mapform);
                return true;
            }
        }
        _DEBUGLOG("No map equipped");
        active_map = nullptr;
        return false;
    }

    void Manager::Refresh()
    {
        Clear();
        z_offset_count = 0;

        if (FindActiveMap())
        {
            FindActiveObjectives();
            state = State::kWaiting;

            if (auto playercondition = (int)settings::Manager::GetSingleton()->Get("iShowPlayer");
                playercondition == 1 || (playercondition == 2 && IsSpellEquipped()))
            {
                DrawPlayerMarker();
            }

            if (auto markercondition = (int)settings::Manager::GetSingleton()->Get("iShowCustom");
                markercondition == 1 || (markercondition == 2 && IsSpellEquipped()))
            {
                DrawCustomMarker();
            }
        }
    }

    void Manager::Clear()
    {
        state = State::kInactive;
        active_objectives.clear();
        seen_targets.clear();
        icon_addons.clear();
        custom_marker.reset();
        player_marker.reset();
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

        //_DEBUGLOG(a_target.)

        for (auto obj : active_objectives)
        {
            for (uint32_t i = 0; i < obj->numTargets; ++i)
            {
                if (obj->targets[i] == a_target)
                {
                    if (obj->ownerQuest)
                    {
                        try
                        {
                            _DEBUGLOG("Quest marker: [{}] {:.30}", obj->ownerQuest->formEditorID,
                                obj->displayText.c_str()
                                //ownerQuest->aliases[a_target->alias]->aliasName
                            );
                        } catch (std ::exception)
                        {
                            _DEBUGLOG("Exception when printing quest ");
                        }
                    }
                    else { _DEBUGLOG("Quest marker added with no parent quest"); }

                    auto type = MapIcon::GetIconType(obj->ownerQuest->GetType());

                    SKSE::GetTaskInterface()->AddTask(
                        [a_pos, type]() { Manager::GetSingleton()->AddMarker(a_pos, type); });

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

    bool Manager::IsSpellEquipped()
    {
        for (const auto isLeft : { true, false })
        {
            if (auto form = RE::PlayerCharacter::GetSingleton()->GetEquippedObject(isLeft);
                form && form->formID == kSpellID)
            {
                return true;
            }
        }
        return false;
    }

    void Manager::DrawPlayerMarker()
    {
        auto pos = RE::PlayerCharacter::GetSingleton()->GetPosition();

        AddMarker({ pos.x, pos.y }, MapIcon::IconType::kPlayer);
    }

    void Manager::DrawCustomMarker()
    {
        if (auto handle = RE::PlayerCharacter::GetSingleton()->playerMapMarker)
        {
            if (auto ref = handle.get())
            {
                auto pos = ref.get()->GetPosition();

                AddMarker({ pos.x, pos.y }, MapIcon::IconType::kCustom);
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
                _DEBUGLOG("Form {:x} is a map", a_form);
                return true;
            }
        }
        _DEBUGLOG("Form {:x} is NOT a map", a_form);
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

    RE::NiTransform Manager::MapToHand(
        RE::NiPoint2 a_coords, bool isLeft, bool a_progressive_offset)
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

        // fix for z fighting, there's a max of 18 markers
        float z_offset = 0.05f;
        if (a_progressive_offset) { z_offset += (z_offset_count++ % 18) * 0.004; }

        // Put the map coordinates into local hand space
        if (isLeft)
        {
            result.translate = lpos + lrot * RE::NiPoint3(a_coords.x, a_coords.y, z_offset);
            result.rotate = lrot;
        }
        else
        {
            result.translate =
                rpos + rrot * RE::NiPoint3(a_coords.x - kRightHandOffsetX, a_coords.y, z_offset);
            result.rotate = rrot;
        }

        return result;
    };

    RE::NiPoint2 TestOverlap(RE::NiPoint2& a_coords, float a_radius)
    {
        RE::NiPoint2 result;
        if (auto right_overlap = a_coords.x + a_radius - kMapWidth; right_overlap > 0)
        {
            result.x = -right_overlap / a_radius;
        }
        else if (auto left_overlap = a_coords.x - a_radius - kMapLowerWidth; left_overlap < 0)
        {
            result.x = -left_overlap / a_radius;
        }
        if (auto top_overlap = a_coords.y + a_radius - kMapHeight; top_overlap > 0)
        {
            result.y = top_overlap / a_radius;
        }
        else if (auto bottom_overlap = a_coords.y - a_radius; bottom_overlap < 0)
        {
            result.y = bottom_overlap / a_radius;
        }
        return result;
    }

    bool IsSkyrim(const HeldMap* a_map) { return a_map->location_form == HoldLocations::Tamriel; }

    bool IsSolstheim(const HeldMap* a_map)
    {
        return a_map->location_form == HoldLocations::Solstheim;
    }
}