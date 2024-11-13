#pragma once

#include <string>
#include <windows.h>

class Setting
{
public:
	bool loaded = false;
	int MaxRainDrops;
	int RainDirection;
	COLORREF RainColor;

	Setting(const int maxRainDrops = 10, const int rainDirection = 3, const COLORREF rainColor = 0x00AAAAAA)
		: MaxRainDrops(maxRainDrops), RainDirection(rainDirection), RainColor(rainColor)
	{
	}
};

class SettingsManager
{
	static SettingsManager* instance;
	std::wstring iniFilePath;
	Setting defaultSetting;

	SettingsManager();
	static std::wstring GetAppDataPath();
	void CreateINIFile() const;

public:
	static SettingsManager* GetInstance();
	void ReadSettings(Setting& setting) const;
	void WriteSettings(const Setting& setting) const;
};
