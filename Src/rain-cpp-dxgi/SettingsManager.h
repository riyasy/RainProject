#pragma once

#include <string>
#include <windows.h>

class Setting {
public:
    int MaxRainDrops;
    int RainDirection;
    COLORREF RainColor;

    Setting(int maxRainDrops = 10, int rainDirection = 3, COLORREF rainColor = 0x00AAAAAA)
        : MaxRainDrops(maxRainDrops), RainDirection(rainDirection), RainColor(rainColor) {}
};

class SettingsManager {
private:
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



