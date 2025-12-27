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
	              COLORREF particleColor, ParticleType partType, bool allowHide);
	static void SubscribeToChange(CallBackWindow* subscriber);
	bool Create();
	void Show() const;
	static LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static std::vector<CallBackWindow*> subscribers;
	HINSTANCE hInstance;
	HWND hDialog;
	int MaxParticles;
	int WindDirection;
	COLORREF ParticleColor;
	ParticleType PartType;
	bool AllowHide;
	static OptionsDialog* pThis;
};
