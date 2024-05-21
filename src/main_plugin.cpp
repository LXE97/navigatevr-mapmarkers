#include "main_plugin.h"

namespace vrmapmarkers
{
	// user settings, documented in .ini
	bool g_debug_print = true;

	// settings
	bool g_left_hand_mode = false;

	void Init()
	{
		ReadConfig(g_ini_path);

		menuchecker::begin();
	}

	void OnGameLoad() { _DEBUGLOG("Load Game: reset state"); }

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
