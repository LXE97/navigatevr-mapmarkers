ScriptName _vrmapmarkers_MCMScript extends SKI_ConfigBase

FormList Property ModSettings Auto

GlobalVariable Property bDebugLog Auto
GlobalVariable Property iBorderStyle Auto
GlobalVariable Property iPlayerStyle Auto
GlobalVariable Property iCustomStyle Auto
GlobalVariable Property iShowSymbol Auto
GlobalVariable Property iShowPlayer Auto
GlobalVariable Property iShowCustom Auto
GlobalVariable Property fBorderScale Auto
GlobalVariable Property fSymbolScale Auto
GlobalVariable Property fRegionalScale Auto
GlobalVariable Property fPlayerScale Auto
GlobalVariable Property fBrightness Auto

Int function GetVersion()
	return 1
endfunction

String dllname = "vrmapmarkers"

String kNewProfileCommand = "Write to New"
String[] ModSettingsStrings
String[] Profiles
String[] ShowPlayerStr
String[] PlayerStyles
String[] SymbolOptions
String[] BorderStyles

; this is just for keeping track of the selection in the profile dropdown
int profileindex = 0

String SelectedProfile = "Default"

int SettingsFile = 0

bool changed = false
bool error = false

; -------------------------------------------------------------------------------------------------
; MAIN EVENTS
; -------------------------------------------------------------------------------------------------
Event OnVersionUpdate(int a_version)
	If (a_version > CurrentVersion)
		debug.trace(ModName + " update from " + CurrentVersion + " to " + a_version)
		OnConfigInit()
	EndIf
EndEvent

event OnConfigInit()
	debug.trace("Initializing " + ModName)

    ;initialize MCM variables
    Pages = New String[1]
    Pages[0] = "Mod Settings"

	; these are the keys for the mod settings in the json, but they must be in the same order as 
	; the settings in the FormList ModSettings 
	ModSettingsStrings = New String[12]
	ModSettingsStrings[0]  = "bDebugLog"
	ModSettingsStrings[1]  = "iBorderStyle"
	ModSettingsStrings[2]  = "iPlayerStyle"
	ModSettingsStrings[3]  = "iCustomStyle"
	ModSettingsStrings[4]  = "iShowSymbol" 
	ModSettingsStrings[5]  = "bShowPlayer"
	ModSettingsStrings[6]  = "bShowCustom"
	ModSettingsStrings[7]  = "fBorderScale"
	ModSettingsStrings[8]  = "fSymbolScale"
	ModSettingsStrings[9]  = "fRegionalScale"
	ModSettingsStrings[10]  = "fPlayerScale"
	ModSettingsStrings[11]  = "fBrightness"

	Profiles = New String[1]
	Profiles[0] = "Default"

	ShowPlayerStr = new String[3]
	ShowPlayerStr[0] = "Never"
	ShowPlayerStr[1] = "Always"
	ShowPlayerStr[2] = "Clairvoyance"

	PlayerStyles = new String[8] 
	PlayerStyles[0] = "Arrow"
	PlayerStyles[1] = "Dovahkiin"
	PlayerStyles[2] = "Magnus"
	PlayerStyles[3] = "Star"
	PlayerStyles[4] = "Scribble"
	PlayerStyles[5] = "Clean Circle"
	PlayerStyles[6] = "Knot"
	PlayerStyles[7] = "Runes"

	SymbolOptions = new String[4] 	
	SymbolOptions[0] = "No Symbols"
	SymbolOptions[1] = "X"
	SymbolOptions[2] = "Factions Only"
	SymbolOptions[3] = "Faction or X (misc)"

	BorderStyles  = new String[5]
	BorderStyles[0] = "No Border"
	BorderStyles[1] = "Scribble"
	BorderStyles[2] = "Clean Circle"
	BorderStyles[3] = "Knot"
	BorderStyles[4] = "Runes"

	ReadJSONFile()
endevent

event OnConfigOpen()
    ReadJSONFile()
	ForcePageReset()
endevent

event OnConfigClose()
    if (changed)
       	WriteJSONFile()
		changed = false
    endif
endevent

; -------------------------------------------------------------------------------------------------
; PAGE DEFINITIONS
; -------------------------------------------------------------------------------------------------
event OnPageReset(String a_page)
	if (error)
		SetCursorFillMode(TOP_TO_BOTTOM)
		AddTextOption("Error: Data/SKSE/Plugins/" + dllname + ".json", "")
		AddTextOption("Configuration file invalid or not found.", "")
		AddTextOption("Please check the console for errors,", "")
		AddTextOption("or reinstall the mod from the archive.", "")
	else
		if (a_page == "" || a_page == "Mod Settings")
			SetCursorFillMode(TOP_TO_BOTTOM)
			; Left side - main mod options
			AddMenuOptionST("OID_iShowPlayer", "Show Player Location", ShowPlayerStr[iShowPlayer.GetValueInt()])
			AddMenuOptionST("OID_iShowCustom", "Show Custom Marker", ShowPlayerStr[iShowCustom.GetValueInt()])
			AddMenuOptionST("OID_iShowSymbol", "Show Symbols", SymbolOptions[iShowSymbol.GetValueInt()])
			AddHeaderOption("Appearance")
			AddMenuOptionST("OID_iBorderStyle", "Quest Border Style", BorderStyles[iBorderStyle.GetValueInt()])
			AddMenuOptionST("OID_iPlayerStyle", "Player Marker Style", PlayerStyles[iPlayerStyle.GetValueInt()])
			AddMenuOptionST("OID_iCustomStyle", "Custom Marker Style", PlayerStyles[iCustomStyle.GetValueInt()])
			AddSliderOptionST("OID_fBrightness", "Brightness", fBrightness.GetValue(), "{1}")
			AddSliderOptionST("OID_fBorderScale", "Border Radius", fBorderScale.GetValue(), "{1}")
			AddSliderOptionST("OID_fSymbolScale", "Symbol Scale", fSymbolScale.GetValue(), "{1}")
			AddSliderOptionST("OID_fRegionalScale", "Regional Scale", fRegionalScale.GetValue(), "{1}")
			AddSliderOptionST("OID_fPlayerScale", "Player Scale", fPlayerScale.GetValue(), "{1}")
	
			; Right side - system options
			SetCursorPosition(1)
			AddHeaderOption("Plugin Settings")
			AddMenuOptionST("OID_ProfileMenu", "Profile", SelectedProfile)
			AddToggleOptionST("OID_bDebugLog", "Enable Debug Log", bDebugLog.GetValueInt())
		
		endif
	endif
endevent

; -------------------------------------------------------------------------------------------------
; GENERIC OPTION EVENTS
; -------------------------------------------------------------------------------------------------

; ----------------------------------------------------------------------------------------------------
; STATE OPTION EVENTS
; ----------------------------------------------------------------------------------------------------
; Styles Page
state OID_iBorderStyle
	event OnMenuOpenST()		
		SetMenuDialogStartIndex(iBorderStyle.GetValueInt())
		SetMenuDialogDefaultIndex(iBorderStyle.GetValueInt())
		SetMenuDialogOptions(BorderStyles)
	endevent
	event OnMenuAcceptST(int index)
		changed = true
		iBorderStyle.SetValueInt(index)
		SetMenuOptionValueST(BorderStyles[index])
	endevent 
endstate

state OID_iPlayerStyle
	event OnMenuOpenST()		
		SetMenuDialogStartIndex(iPlayerStyle.GetValueInt())
		SetMenuDialogDefaultIndex(iPlayerStyle.GetValueInt())
		SetMenuDialogOptions(PlayerStyles)
	endevent
	event OnMenuAcceptST(int index)
		changed = true
		iPlayerStyle.SetValueInt(index)
		SetMenuOptionValueST(PlayerStyles[index])
	endevent 
endstate

state OID_iCustomStyle
	event OnMenuOpenST()		
		SetMenuDialogStartIndex(iCustomStyle.GetValueInt())
		SetMenuDialogDefaultIndex(iCustomStyle.GetValueInt())
		SetMenuDialogOptions(PlayerStyles)
	endevent
	event OnMenuAcceptST(int index)
		changed = true
		iCustomStyle.SetValueInt(index)
		SetMenuOptionValueST(PlayerStyles[index])
	endevent 
endstate

; Main page
state OID_bDebugLog
	event OnHighlightST()
		SetInfoText("Print extra debug info, see: Documents/My Games/Skyrim VR/SKSE/" + dllname + ".log")
	endevent
	Event OnSelectST()
		changed = true
		if bDebugLog.GetValueInt()
            SetToggleOptionValueST(false)
			bDebugLog.SetValueInt(0)
        Else
            SetToggleOptionValueST(true)
			bDebugLog.SetValueInt(1)
        endif
    endevent
endstate

state OID_fBorderScale
	event OnHighlightST()
		SetInfoText("Size of the marker outlines and custom marker")
	endevent
	event OnSliderOpenST()
		SetSliderDialogStartValue(fBorderScale.GetValue())
		SetSliderDialogDefaultValue(fBorderScale.GetValue())
		SetSliderDialogRange(0.5, 4.0)
		SetSliderDialogInterval(0.1)
	endEvent
	event OnSliderAcceptST(float value)
		changed = true
		fBorderScale.SetValue(value)
		SetSliderOptionValueST(value, "{1}")
	endEvent
endState

state OID_fSymbolScale
	event OnHighlightST()
		SetInfoText("Size of the faction symbols")
	endevent
	event OnSliderOpenST()
		SetSliderDialogStartValue(fSymbolScale.GetValue())
		SetSliderDialogDefaultValue(fSymbolScale.GetValue())
		SetSliderDialogRange(0.5, 4.0)
		SetSliderDialogInterval(0.1)
	endEvent
	event OnSliderAcceptST(float value)
		changed = true
		fSymbolScale.SetValue(value)
		SetSliderOptionValueST(value, "{1}")
	endEvent
endState

state OID_fRegionalScale
	event OnHighlightST()
		SetInfoText("Magnify all markers by this amount on regional (zoomed-in) maps")
	endevent
	event OnSliderOpenST()
		SetSliderDialogStartValue(fRegionalScale.GetValue())
		SetSliderDialogDefaultValue(fRegionalScale.GetValue())
		SetSliderDialogRange(0.5, 4.0)
		SetSliderDialogInterval(0.1)
	endEvent
	event OnSliderAcceptST(float value)
		changed = true
		fRegionalScale.SetValue(value)
		SetSliderOptionValueST(value, "{1}")
	endEvent
endState

state OID_fPlayerScale
	event OnHighlightST()
		SetInfoText("Size of the player marker")
	endevent
	event OnSliderOpenST()
		SetSliderDialogStartValue(fPlayerScale.GetValue())
		SetSliderDialogDefaultValue(fPlayerScale.GetValue())
		SetSliderDialogRange(0.5, 4.0)
		SetSliderDialogInterval(0.1)
	endEvent
	event OnSliderAcceptST(float value)
		changed = true
		fPlayerScale.SetValue(value)
		SetSliderOptionValueST(value, "{1}")
	endEvent
endState

state OID_fBrightness
	event OnHighlightST()
		SetInfoText("Increase the brightness of all markers")
	endevent
	event OnSliderOpenST()
		SetSliderDialogStartValue(fBrightness.GetValue())
		SetSliderDialogDefaultValue(fBrightness.GetValue())
		SetSliderDialogRange(0.0, 4.0)
		SetSliderDialogInterval(0.1)
	endEvent
	event OnSliderAcceptST(float value)
		changed = true
		fBrightness.SetValue(value)
		SetSliderOptionValueST(value, "{1}")
	endEvent
endState

state OID_iShowPlayer
	event OnHighlightST()
		SetInfoText("'Clairvoyance' condition is true when you have that spell equipped")
	endevent
	event OnMenuOpenST()		
		SetMenuDialogStartIndex(iShowPlayer.GetValueInt())
		SetMenuDialogDefaultIndex(iShowPlayer.GetValueInt())
		SetMenuDialogOptions(ShowPlayerStr)
	endevent
	event OnMenuAcceptST(int index)
		changed = true
		iShowPlayer.SetValueInt(index)
		SetMenuOptionValueST(ShowPlayerStr[index])
	endevent 
endstate

state OID_iShowCustom
	event OnHighlightST()
		SetInfoText("'Clairvoyance' condition is true when you have that spell equipped")
	endevent
	event OnMenuOpenST()		
		SetMenuDialogStartIndex(iShowCustom.GetValueInt())
		SetMenuDialogDefaultIndex(iShowCustom.GetValueInt())
		SetMenuDialogOptions(ShowPlayerStr)
	endevent
	event OnMenuAcceptST(int index)
		changed = true
		iShowCustom.SetValueInt(index)
		SetMenuOptionValueST(ShowPlayerStr[index])
	endevent 
endstate

state OID_iShowSymbol
	event OnHighlightST()
		SetInfoText("Displays a symbol pertaining to the quest type in addition to the border")
	endevent
	event OnMenuOpenST()		
		SetMenuDialogStartIndex(iShowSymbol.GetValueInt())
		SetMenuDialogDefaultIndex(iShowSymbol.GetValueInt())
		SetMenuDialogOptions(SymbolOptions)
	endevent
	event OnMenuAcceptST(int index)
		changed = true
		iShowSymbol.SetValueInt(index)
		SetMenuOptionValueST(SymbolOptions[index])
	endevent 
endstate

state OID_ProfileMenu
	event OnHighlightST()
		SetInfoText("Changes are only saved when the MCM is closed")
	endevent
	event OnMenuOpenST()		
		SetMenuDialogStartIndex(profileindex)
		SetMenuDialogDefaultIndex(profileindex)
		SetMenuDialogOptions(Profiles)
	endevent
	event OnMenuAcceptST(int index)
		profileindex = index
		if Profiles[index] != SelectedProfile
			SelectedProfile = Profiles[index]

			if (SelectedProfile != kNewProfileCommand)
				ReadJSONFile()
				ForcePageReset()
			else
				; always want to write out settings if new profile is created
				changed = true
			endif
		endif
		SetMenuOptionValueST(Profiles[index])
	endevent 
endstate

;-----------------------------------------------------------------------------------------------------
; HELPER FUNCTIONS
; ----------------------------------------------------------------------------------------------------

function SettingsToJSON(bool write, int _profile)	
	; process all settings in profile that correspond to the globalvars in our formlist
	int i = ModSettings.GetSize() - 1
	while i >= 0
		GlobalVariable localsetting = ModSettings.GetAt(i) as GlobalVariable

		if (write)
			String type = StringUtil.GetNthChar(ModSettingsStrings[i], 0)
			if (type == "f")
				JMap.setFlt(_profile, ModSettingsStrings[i], localsetting.GetValue())
			else 
				JMap.setInt(_profile, ModSettingsStrings[i], localsetting.GetValueInt())
			endif
		else 
			float read = JMap.getFlt(_profile, ModSettingsStrings[i])
			if (read > 0)
				localsetting.SetValue(read)
			endif
		endif

		i = i - 1
	endwhile
endfunction

function ReadJSONFile()
	; get settings and select profile
    SettingsFile = JValue.readFromFile("Data/SKSE/Plugins/" + dllname + ".json")

	if (SettingsFile == 0)
		error = true
	else
		; check if profile exists, choose Default if not
		int Profile = JMap.GetObj(SettingsFile, SelectedProfile)

		if (Profile == 0)
			SelectedProfile = "Default"
			Profile = JMap.GetObj(SettingsFile, "Default")
			if (Profile == 0)
				error = true
			endif
		endif

		; populate Profiles string list, adding the Make New Profile command
		JMap.SetObj(SettingsFile, kNewProfileCommand, Profile) 
		Profiles = JMap.allKeysPArray(SettingsFile)

		if (error == false)
			SettingsToJSON(false, Profile)
			JValue.zeroLifetime(Profile)
		endif
	endif

	JValue.zeroLifetime(SettingsFile)
endfunction

function WriteJSONFile()
	; get settings
    SettingsFile = JValue.readFromFile("Data/SKSE/Plugins/" + dllname + ".json")

	if (SettingsFile == 0)
		error = true
	else
		if (SelectedProfile == kNewProfileCommand)
			; create new profile
			int _default = JMap.GetObj(SettingsFile, "Default")
			int copy = JValue.deepCopy(_default)

			; decide on new profile key
			int count = JMap.count(SettingsFile)
			string newname = "Profile " + count
			int checker = JMap.getObj(SettingsFile, newname)
			; loop until "checker" is not an existing obj
			while checker > 0
				count = count + 1
				newname = "Profile " + count
				checker = JMap.getObj(SettingsFile, newname)
			endwhile
			SelectedProfile = newname

			; create entry in our copy of json file
			JMap.setObj(SettingsFile, newname, copy)
		endif

		int Profile = JMap.GetObj(SettingsFile, SelectedProfile)
		
		; retry with default profile if Profile doesn't exist
		if (Profile == 0)
			SelectedProfile = "Default"
			Profile = JMap.GetObj(SettingsFile, "Default")
			if (Profile == 0)
				error = true
			endif
		endif

		if (Profile != 0)
			; copy local settings to profile
			SettingsToJSON(true, Profile)

			JValue.writeToFile(SettingsFile, "Data/SKSE/Plugins/" + dllname + ".json")

			JValue.zeroLifetime(Profile)			
		endif

		JValue.zeroLifetime(SettingsFile)
	endif
endfunction