#include "SettingsManager.h"

#include <windows.h>
#include <string>
#include <shlobj.h> // For SHGetFolderPath
#include <iostream>

// Initialize the singleton instance to nullptr
SettingsManager* SettingsManager::instance = nullptr;

SettingsManager::SettingsManager() : defaultSetting(10, 3, 0x00AAAAAA, RAIN)
{
	const std::wstring appDataPath = GetAppDataPath();
	iniFilePath = appDataPath + L"\\let-it-rain.ini";
}

std::wstring SettingsManager::GetAppDataPath()
{
	wchar_t path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
	{
		return { std::wstring(path) };
	}
	return L"";
}

void SettingsManager::CreateINIFile() const
{
	//CreateDirectory(GetAppDataPath().c_str(), NULL); // Create directory if it doesn't exist
	WritePrivateProfileString(L"Settings", L"MaxParticles", std::to_wstring(defaultSetting.MaxParticles).c_str(),
	                          iniFilePath.c_str());
	WritePrivateProfileString(L"Settings", L"WindSpeed", std::to_wstring(defaultSetting.WindSpeed).c_str(),
	                          iniFilePath.c_str());
	wchar_t colorBuffer[10];
	swprintf_s(colorBuffer, 10, L"%08X", defaultSetting.ParticleColor);
	WritePrivateProfileString(L"Settings", L"ParticleColor", colorBuffer, iniFilePath.c_str());

	WritePrivateProfileString(L"Settings", L"ParticleType", std::to_wstring(defaultSetting.PartType).c_str(),
		iniFilePath.c_str());
}

SettingsManager* SettingsManager::GetInstance()
{
	if (!instance)
	{
		instance = new SettingsManager();
	}
	return instance;
}

void SettingsManager::ReadSettings(Setting& setting) const
{
	if (GetFileAttributes(iniFilePath.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		CreateINIFile(); // File does not exist, create with defaults
	}

	setting.MaxParticles = GetPrivateProfileInt(L"Settings", L"MaxParticles", defaultSetting.MaxParticles,
	                                            iniFilePath.c_str());
	setting.WindSpeed = GetPrivateProfileInt(L"Settings", L"WindSpeed", defaultSetting.WindSpeed,
	                                             iniFilePath.c_str());

	wchar_t colorBuffer[10];
	GetPrivateProfileString(L"Settings", L"ParticleColor", nullptr, colorBuffer, 10, iniFilePath.c_str());
	setting.ParticleColor = colorBuffer[0] ? wcstoul(colorBuffer, nullptr, 16) : defaultSetting.ParticleColor;

	setting.PartType = static_cast<ParticleType>(GetPrivateProfileInt(L"Settings", L"ParticleType", defaultSetting.PartType,
	                                                                  iniFilePath.c_str()));

	// Update missing values in INI file
	WriteSettings(setting);
}

void SettingsManager::WriteSettings(const Setting& setting) const
{
	WritePrivateProfileString(L"Settings", L"MaxParticles", std::to_wstring(setting.MaxParticles).c_str(),
	                          iniFilePath.c_str());
	WritePrivateProfileString(L"Settings", L"WindSpeed", std::to_wstring(setting.WindSpeed).c_str(),
	                          iniFilePath.c_str());
	wchar_t colorBuffer[10];
	swprintf_s(colorBuffer, 10, L"%08X", setting.ParticleColor);
	WritePrivateProfileString(L"Settings", L"ParticleColor", colorBuffer, iniFilePath.c_str());
	WritePrivateProfileString(L"Settings", L"ParticleType", std::to_wstring(setting.PartType).c_str(),
		iniFilePath.c_str());
}
