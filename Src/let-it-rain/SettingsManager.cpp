#include "SettingsManager.h"

#include <windows.h>
#include <string>
#include <shlobj.h> // For SHGetFolderPath
#include <iostream>

// Initialize the singleton instance to nullptr
SettingsManager* SettingsManager::instance = nullptr;

SettingsManager::SettingsManager() : defaultSetting(10, 3, 0x00AAAAAA)
{
	std::wstring appDataPath = GetAppDataPath();
	iniFilePath = appDataPath + L"\\let-it-rain.ini";
}

std::wstring SettingsManager::GetAppDataPath()
{
	wchar_t path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
	{
		return std::wstring(path);
	}
	return L"";
}

void SettingsManager::CreateINIFile() const
{
	//CreateDirectory(GetAppDataPath().c_str(), NULL); // Create directory if it doesn't exist
	WritePrivateProfileString(L"Settings", L"MaxRainDrops", std::to_wstring(defaultSetting.MaxRainDrops).c_str(),
	                          iniFilePath.c_str());
	WritePrivateProfileString(L"Settings", L"RainDirection", std::to_wstring(defaultSetting.RainDirection).c_str(),
	                          iniFilePath.c_str());
	wchar_t colorBuffer[10];
	swprintf_s(colorBuffer, 10, L"%08X", defaultSetting.RainColor);
	WritePrivateProfileString(L"Settings", L"RainColor", colorBuffer, iniFilePath.c_str());
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

	setting.MaxRainDrops = GetPrivateProfileInt(L"Settings", L"MaxRainDrops", defaultSetting.MaxRainDrops,
	                                            iniFilePath.c_str());
	setting.RainDirection = GetPrivateProfileInt(L"Settings", L"RainDirection", defaultSetting.RainDirection,
	                                             iniFilePath.c_str());

	wchar_t colorBuffer[10];
	GetPrivateProfileString(L"Settings", L"RainColor", nullptr, colorBuffer, 10, iniFilePath.c_str());
	setting.RainColor = colorBuffer[0] ? wcstoul(colorBuffer, nullptr, 16) : defaultSetting.RainColor;

	// Update missing values in INI file
	WriteSettings(setting);
}

void SettingsManager::WriteSettings(const Setting& setting) const
{
	WritePrivateProfileString(L"Settings", L"MaxRainDrops", std::to_wstring(setting.MaxRainDrops).c_str(),
	                          iniFilePath.c_str());
	WritePrivateProfileString(L"Settings", L"RainDirection", std::to_wstring(setting.RainDirection).c_str(),
	                          iniFilePath.c_str());
	wchar_t colorBuffer[10];
	swprintf_s(colorBuffer, 10, L"%08X", setting.RainColor);
	WritePrivateProfileString(L"Settings", L"RainColor", colorBuffer, iniFilePath.c_str());
}
