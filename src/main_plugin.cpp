#include "main_plugin.h"

#include "settings.h"

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

        mapmarker::UpdatePlayerMarker();
    }

    void Init()
    {
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

        settings::Manager::GetSingleton()->Init("vrmapmarkers", 0x801);

        // Mapmarker initialization
        mapmarker::g_dawnguard_faction =
            helper::GetForm<RE::TESFaction>(g_dawnguard_faction_id, g_dawnguard_plugin_name);

        mapmarker::g_stormcloak_faction =
            RE::TESForm::LookupByID<RE::TESFaction>(g_stormcloak_faction_id);

        mapmarker::g_mod_index = g_mod_index;

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
        _DEBUGLOG("Load Game: reset state");
        ArtAddonManager::GetSingleton()->OnGameLoad();

        settings::Manager::GetSingleton()->PrintSettings();

        mapmarker::RefreshSettings();

        mapmarker::ClearMarkers();
        if (RE::PlayerCharacter::GetSingleton()->GetCurrent3D()) { mapmarker::UpdateMapMarkers(); }
    }

    void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn)
    {
        if (!evn->opening && std::strcmp(evn->menuName.data(), "Journal Menu") == 0 ||
            (mapmarker::g_show_playermarker && std::strcmp(evn->menuName.data(), "MapMenu") == 0))
        {
            g_debug_print = settings::Manager::GetSingleton()->Get("bDebugLog");

            settings::Manager::GetSingleton()->PrintSettings();
            mapmarker::RefreshSettings();
            mapmarker::ClearMarkers();
            mapmarker::UpdateMapMarkers();
        }
    }

    void OnEquipEvent(const RE::TESEquipEvent* event)
    {
        if (event->actor &&
            event->actor.get() == RE::PlayerCharacter::GetSingleton()->AsReference())
        {
            if (helper::GetFormIndex(event->baseObject) == g_mod_index &&
                ((event->baseObject & 0xffff) != 0xe09d) &&
                ((event->baseObject & 0xffff) != 0x0d62))  // exclusions for compass
            {
                _DEBUGLOG("player {} {:x}", event->equipped ? "equipped" : "unequipped",
                    event->baseObject);

                mapmarker::ClearMarkers();
                if (event->equipped) { mapmarker::UpdateMapMarkers(); }
            }
        }
    }
}
