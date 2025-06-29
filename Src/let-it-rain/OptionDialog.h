#pragma once

#include <windows.h>
#include <commctrl.h>
#include "CallBackWindow.h"
#include <vector>

#pragma comment(lib, "comctl32.lib")

class OptionsDialog
{
public:
	OptionsDialog(HINSTANCE hInstance, int maxParticles, int windDirection,
	              COLORREF particleColor, ParticleType partType, int lightningFreq, int lightningIntensity,
	              bool enableSnowWind = false, int snowWindIntensity = 25, int snowWindVariability = 50);
	static void SubscribeToChange(CallBackWindow* subscriber);
	bool Create();
	void Show() const;
	static LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// Helper methods for notifications
	static void NotifyParticleCountChange(int newValue);
	static void NotifyWindDirectionChange(int newValue);
	static void NotifyParticleColorChange(COLORREF newColor);
	static void NotifyParticleTypeChange(ParticleType newType);
	static void NotifyLightningFrequencyChange(int newValue);
	static void NotifyLightningIntensityChange(int newValue);
	static void NotifyEnableSnowWind(bool enabled);
	static void NotifySnowWindIntensity(int newValue);
	static void NotifySnowWindVariability(int newValue);
	
	// UI helper methods
	void CenterDialog() const;
	static void ShowColorChooserDialog(HWND hWnd);
	static void UpdateSnowWindControlsState(HWND hWnd, bool enabled);
	
	// Member variables
	static std::vector<CallBackWindow*> subscribers;
	HINSTANCE hInstance;
	HWND hDialog;
	int MaxParticles;
	int WindDirection;
	COLORREF ParticleColor;
	ParticleType PartType;
	int LightningFrequency;
	int LightningIntensity;
	bool EnableSnowWind;
	int SnowWindIntensity;
	int SnowWindVariability;
	static OptionsDialog* pThis;
};
