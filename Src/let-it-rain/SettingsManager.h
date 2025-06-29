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
	
	// Lightning settings
	int LightningFrequency;  // 0-100 scale, 50 = default
	int LightningIntensity;  // 0-100 scale, 50 = default

	// Snow wind randomness settings
	bool EnableSnowWind;      // Whether to enable random wind for snow
	int SnowWindIntensity;    // 0-100 scale, how strong the wind is
	int SnowWindVariability;  // 0-100 scale, how frequently the wind changes

	explicit Setting(const int maxParticles = 10, 
		const int windSpeed = 3, 
		const COLORREF ParticleColor = 0x00AAAAAA, 
		const ParticleType partType = RAIN,
		const int lightningFrequency = 50,
		const int lightningIntensity = 50,
		const bool enableSnowWind = false,
		const int snowWindIntensity = 25,
		const int snowWindVariability = 50)
		: MaxParticles(maxParticles), WindSpeed(windSpeed), ParticleColor(ParticleColor), PartType(partType),
		  LightningFrequency(lightningFrequency), LightningIntensity(lightningIntensity),
		  EnableSnowWind(enableSnowWind), SnowWindIntensity(snowWindIntensity), SnowWindVariability(snowWindVariability)
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
