#pragma once

#include <string>
#include <windows.h>

enum ParticleType
{
	RAIN = 0,
	SNOW = 1,
};

class Setting
{
public:
	bool loaded = false;
	int MaxParticles;
	int WindSpeed;
	COLORREF ParticleColor;
	ParticleType PartType;
	bool StartWithWindows;
	bool AllowHide;

	explicit Setting(const int maxParticles = 10,
		const int windSpeed = 3,
		const COLORREF ParticleColor = 0x00AAAAAA,
		const ParticleType partType = RAIN,
		const bool startWithWindows = false,
		const bool allowHide = false)
		: MaxParticles(maxParticles), WindSpeed(windSpeed), ParticleColor(ParticleColor), PartType(partType), StartWithWindows(startWithWindows), AllowHide(allowHide)
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
	static bool IsStartupEnabled();
	static void SetStartupEnabled(bool enabled);
};
