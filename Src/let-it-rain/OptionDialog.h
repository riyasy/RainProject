#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include "CallBackWindow.h"

#pragma comment(lib, "comctl32.lib")

class OptionsDialog
{
public:
	OptionsDialog(HINSTANCE hInstance, CallBackWindow* pRainWindow, int maxRainDrops, int rainDirection,
	              COLORREF rainColor);
	bool Create();
	void Show() const;
	static LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE hInstance;
	HWND hDialog;
	CallBackWindow* pRainWindow;
	int MaxRainDrops;
	int RainDirection;
	COLORREF RainColor;
	static OptionsDialog* pThis;
};
