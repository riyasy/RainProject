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
	              COLORREF particleColor, ParticleType partType, bool startWithWindows, bool allowHide,
	              bool simpleSnowHeap);
	static void SubscribeToChange(CallBackWindow* subscriber);
	bool Create();
	void Show() const;
	// Windows switched between light and dark while we were up. Driven from
	// DisplayWindow's WM_SETTINGCHANGE rather than our own, so the uxtheme
	// colour cache is refreshed before anything here reads it.
	void ApplyTheme() const;
	static LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static std::vector<CallBackWindow*> subscribers;
	HINSTANCE hInstance;
	HWND hDialog;
	int MaxParticles;
	int WindDirection;
	COLORREF ParticleColor;
	ParticleType PartType;
	bool StartWithWindows;
	bool AllowHide;
	bool SimpleSnowHeap;
	static OptionsDialog* pThis;
};
