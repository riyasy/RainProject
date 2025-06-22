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
	              COLORREF particleColor, ParticleType partType, int lightningFreq, int lightningIntensity);
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
	
	// UI helper methods
	void CenterDialog() const;
	static void ShowColorChooserDialog(HWND hWnd);
	
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
	static OptionsDialog* pThis;
};
