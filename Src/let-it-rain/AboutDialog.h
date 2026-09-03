#pragma once

#include <windows.h>

// The About box. A dialog of its own rather than a second tab on the settings
// dialog: a tab control paints its pane with the theme's tab-body texture while
// sibling controls get the plain dialog brush, so the two never quite match —
// and it has no dark variant at all. Two dialogs and a third tray menu item
// cost less than fighting either.
//
// Modeless and created once at startup, like OptionsDialog, so opening it never
// blocks the render loop. Closing hides it; nothing destroys it before exit.
class AboutDialog
{
public:
	explicit AboutDialog(HINSTANCE hInstance);
	bool Create();
	void Show() const;
	// Windows switched between light and dark while we were up. Driven from
	// DisplayWindow's WM_SETTINGCHANGE rather than our own, so the uxtheme
	// colour cache is refreshed before anything here reads it.
	void ApplyTheme() const;

private:
	static LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HINSTANCE hInstance;
	HWND hDialog;
	static AboutDialog* pThis;
};
