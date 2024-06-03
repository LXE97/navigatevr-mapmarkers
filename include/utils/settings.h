#pragma once

#include <any>

namespace settings
{
	class Manager
	{
	public:
		static Manager* GetSingleton()
		{
			static Manager singleton;
			return &singleton;
		}

		void Init(std::string a_mod_name, RE::FormID a_profile_globalvar = 0);

		const float Get(const std::string a_setting_editorid);

		void PrintSettings() const;

	private:
		Manager() = default;
		~Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;
		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

		RE::BGSListForm*                                mod_settings = nullptr;;
		std::unordered_map<std::string, RE::TESGlobal*> global_vars;
	};
}