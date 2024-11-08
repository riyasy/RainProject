#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include "CallBackWindow.h"
#include <vector>

#pragma comment(lib, "comctl32.lib")

class OptionsDialog
{
public:
	OptionsDialog(HINSTANCE hInstance, int maxRainDrops, int rainDirection,
	              COLORREF rainColor);
	static void SubscribeToChange(CallBackWindow* subscriber);
	bool Create();
	void Show() const;
	static LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static std::vector<CallBackWindow*> subscribers;
	HINSTANCE hInstance;
	HWND hDialog;
	int MaxRainDrops;
	int RainDirection;
	COLORREF RainColor;
	static OptionsDialog* pThis;
};
