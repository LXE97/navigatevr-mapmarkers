#include "main_plugin.h"

#include <chrono>

namespace vrmapmarkers
{
    using namespace art_addon;

    // User settings, documented in .ini
    bool g_debug_print = true;

    // Settings
    bool g_left_hand_mode = false;

    // Resources
    int                   g_mod_index = 0;
    const RE::FormID      g_stormcloak_faction_id = 0x2bf9b;
    const RE::FormID      g_dawnguard_faction_id = 0x14217;
    constexpr const char* g_dawnguard_plugin_name = "Dawnguard.esm";

    void PlayerUpdate()
    {
        static auto mgr = ArtAddonManager::GetSingleton();
        mgr->Update();
    }

    void Init()
    {
        if (auto file = RE::TESDataHandler::GetSingleton()->LookupModByName(g_plugin_name))
        {
            g_mod_index = file->GetPartialIndex();
        }
        else
        {
            SKSE::log::error(
                "NavigateVR ESP not found, please ensure you enabled the mod and did not rename "
                "the plugin file");
            return;
        }

        helper::InstallPlayerUpdateHook(PlayerUpdate);

        // Mapmarker initialization
        mapmarker::g_dawnguard_faction =
            helper::GetForm<RE::TESFaction>(g_dawnguard_faction_id, g_dawnguard_plugin_name);

        mapmarker::g_stormcloak_faction =
            RE::TESForm::LookupByID<RE::TESFaction>(g_stormcloak_faction_id);

        mapmarker::g_mod_index = g_mod_index;

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

        mapmarker::ClearMarkers();
        if (RE::PlayerCharacter::GetSingleton()->GetCurrent3D()) { mapmarker::UpdateMapMarkers(); }
    }

    void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn)
    {
        if (!evn->opening && std::strcmp(evn->menuName.data(), "Journal Menu") == 0)
        {
            ReadConfig(g_ini_path);

            mapmarker::ClearMarkers();
            mapmarker::UpdateMapMarkers();
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

                mapmarker::ClearMarkers();
                if (event->equipped) { mapmarker::UpdateMapMarkers(); }
            }
        }
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
                    mapmarker::g_use_symbols = helper::ReadIntFromIni(config, "bUseSymbols");
                    mapmarker::selected_border = helper::ReadIntFromIni(config, "iBorder") % 4;
                    mapmarker::g_border_scale =
                        std::clamp(helper::ReadFloatFromIni(config, "fBorderScale"), 0.2f, 10.f);
                    mapmarker::g_symbol_scale =
                        std::clamp(helper::ReadFloatFromIni(config, "fSymbolScale"), 0.2f, 10.f);
                    mapmarker::g_regional_scale =
                        std::clamp(helper::ReadFloatFromIni(config, "fRegionalScale"), 0.2f, 10.f);
                    mapmarker::g_show_playermarker =
                        helper::ReadIntFromIni(config, "bShowCustomMarker");
                    mapmarker::g_rotate_border =
                        helper::ReadIntFromIni(config, "bRandomAdjustment");

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
