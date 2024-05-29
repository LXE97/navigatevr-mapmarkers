#include "main_plugin.h"

#include "hooks.h"

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
        hooks::Install();
        // Populate resource references
        if (auto file = RE::TESDataHandler::GetSingleton()->LookupModByName(g_plugin_name))
        {
            g_mod_index = file->GetPartialIndex();
            SKSE::log::trace("NavigateVR ESP index: {:x}", g_mod_index);
            if (g_mod_index == 0xff || !g_mod_index)
            {
                SKSE::log::error("NavigateVR ESP not activated, aborting");
                return;
            }
        }
        else
        {
            SKSE::log::error(
                "NavigateVR ESP not found, please ensure you did not rename the plugin file");
            return;
        }

        // Mapmarker initialization
        mapmarker::g_dawnguard_faction =
            helper::GetForm<RE::TESFaction>(g_dawnguard_faction_id, g_dawnguard_plugin_name);

        mapmarker::g_stormcloak_faction =
            RE::TESForm::LookupByID<RE::TESFaction>(g_stormcloak_faction_id);

        mapmarker::g_mod_index = g_mod_index;

        ReadConfig(g_ini_path);

        menuchecker::begin();

        helper::InstallPlayerUpdateHook(PlayerUpdate);

        auto equip_sink = EventSink<RE::TESEquipEvent>::GetSingleton();
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(equip_sink);
        equip_sink->AddCallback(OnEquipEvent);

        auto menu_sink = EventSink<RE::MenuOpenCloseEvent>::GetSingleton();
        RE::UI::GetSingleton()->AddEventSink(menu_sink);
        menu_sink->AddCallback(OnMenuOpenClose);
    }

    void OnGameLoad()
    {
        ArtAddonManager::GetSingleton()->OnGameLoad();
        mapmarker::Manager::GetSingleton()->Refresh();
    }

    void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn)
    {
        if (!evn->opening && (std::strcmp(evn->menuName.data(), "Journal Menu") == 0 ||
            std::strcmp(evn->menuName.data(), "MapMenu") == 0))
        {
            ReadConfig(g_ini_path);

            mapmarker::Manager::GetSingleton()->Refresh();

        }
    }

    void OnEquipEvent(const RE::TESEquipEvent* event)
    {
        if (event->actor &&
            event->actor.get() == RE::PlayerCharacter::GetSingleton()->AsReference())
        {
            if (helper::GetFormIndex(event->baseObject) == g_mod_index)
            {
                mapmarker::Manager::GetSingleton()->Refresh();
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
                    auto &settings = mapmarker::Manager::GetSingleton()->SetSettings();
                    settings.use_symbols = helper::ReadIntFromIni(config, "bUseSymbols");
                    settings.selected_border = helper::ReadIntFromIni(config, "iBorder") % 4;
                    settings.border_scale =
                        std::clamp(helper::ReadFloatFromIni(config, "fBorderScale"), 0.2f, 10.f);
                    settings.symbol_scale =
                        std::clamp(helper::ReadFloatFromIni(config, "fSymbolScale"), 0.2f, 10.f);
                    settings.regional_scale =
                        std::clamp(helper::ReadFloatFromIni(config, "fRegionalScale"), 0.2f, 10.f);
                    settings.show_custom =
                        helper::ReadIntFromIni(config, "bShowCustomMarker");
                    settings.show_player = helper::ReadIntFromIni(config, "bShowPlayer");

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
