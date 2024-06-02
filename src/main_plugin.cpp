#include "main_plugin.h"

#include "hooks.h"
#include "settings.h"

#include <chrono>

namespace vrmapmarkers
{
    using namespace art_addon;

    // User settings
    bool g_debug_print = true;

    // Resources
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
        helper::InstallPlayerUpdateHook(PlayerUpdate);

        // Populate resource references
        if (auto file = RE::TESDataHandler::GetSingleton()->LookupModByName(g_plugin_name))
        {
            int mod_index = file->GetPartialIndex();
            SKSE::log::trace("NavigateVR ESP index: {:x}", mod_index);
            if (mod_index == 0xff || !mod_index)
            {
                SKSE::log::error("NavigateVR ESP not activated, aborting");
                return;
            }
            else { mapmarker::g_base_plugin = file; }
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
        settings::Manager::GetSingleton()->Init(g_mod_name, g_settings_formlist);
        g_debug_print = settings::Manager::GetSingleton()->Get("bDebugLog");
        if (g_debug_print) { settings::Manager::GetSingleton()->PrintSettings(); }

        ArtAddonManager::GetSingleton()->OnGameLoad();
        mapmarker::Manager::GetSingleton()->Refresh();
    }

    void OnMenuOpenClose(RE::MenuOpenCloseEvent const* evn)
    {
        if (!evn->opening &&
            (std::strcmp(evn->menuName.data(), "Journal Menu") == 0 ||
                std::strcmp(evn->menuName.data(), "MapMenu") == 0))
        {
            mapmarker::Manager::GetSingleton()->Refresh();
            g_debug_print = settings::Manager::GetSingleton()->Get("bDebugLog");
            if (g_debug_print) { settings::Manager::GetSingleton()->PrintSettings(); }
        }
    }

    void OnEquipEvent(const RE::TESEquipEvent* event)
    {
        if (event->actor &&
            event->actor.get() == RE::PlayerCharacter::GetSingleton()->AsReference())
        {
            if (mapmarker::Manager::GetSingleton()->IsMap(event->baseObject))
            {
                mapmarker::Manager::GetSingleton()->Refresh();
            }
        }
    }
}
