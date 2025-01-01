#pragma once

#include <string>
#include <windows.h>

// setup default rate for Snow and Rain
#define MAX_PARTICLES 75

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

	explicit Setting(const int maxParticles = 10, 
		const int windSpeed = 3, 
		const COLORREF ParticleColor = 0x00AAAAAA, 
		const ParticleType partType = RAIN)
		: MaxParticles(maxParticles), WindSpeed(windSpeed), ParticleColor(ParticleColor), PartType(partType)
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
