#pragma once

#include <windows.h>

// Dark mode: the tray context menu, the Settings dialog and the About dialog.
//
// Ported from MinClock (Src\Clock\winutil.cpp and dlgchrome.cpp). Two halves,
// because Windows themes them by two different mechanisms:
//
//   Menus   - process-wide, via InitDarkMode(). Nothing at the TrackPopupMenu
//             call site changes.
//   Dialogs - per control, via ThemeDialog(), plus each dialog's WM_CTLCOLOR*
//             handlers. Windows themes a control's *glyphs* when it is told
//             which dark theme class to use, but never paints the surface
//             behind them; that surface is what WM_CTLCOLOR* is for.
//
// Light mode passes nullptr to SetWindowTheme, which means "no override" — so
// turning dark mode off in Windows restores the stock dialog, not an imitation.
//
// Deliberately absent: anything that paints a control by hand. An earlier
// attempt themed a tab control that way and never stopped looking wrong, which
// is why Settings and About are two plain dialogs and there is no tab control
// left to theme. The one exception below earns its place — see it.

const COLORREF DARK_BG = RGB(32, 32, 32);
const COLORREF DARK_FG = RGB(255, 255, 255);

// Opt the process into dark menus and resolve the uxtheme ordinals the rest of
// this header needs. Call once at startup, before any menu or window exists.
// A no-op on anything older than Windows 10 1809.
void InitDarkMode();

// Whether Windows is in dark mode. False on any build too old to carry the
// ordinal, which is the right answer there — no dark mode to follow.
bool IsDarkMode();

// Re-resolve the cached light/dark policy and flush the menu theme, in that
// order. The order is the whole point — see the definition.
void ReflushMenuTheme();

// The background brush for the current theme: the dark one ours, the light one
// the system's. What the WM_CTLCOLOR* handlers return.
HBRUSH ThemeBrush();

// Point one control at its dark theme class, or clear the override in light
// mode. "DarkMode_CFD" for combo boxes and edits, "DarkMode_Explorer" for
// everything else.
void ThemeControl(HWND control, const WCHAR* darkClass);

// Darken a window's title bar, which DWM owns and no theme class can reach.
void ThemeCaption(HWND window);

// Caption plus every child control, and a repaint. Safe to call again when the
// theme changes under an open dialog.
void ThemeDialog(HWND dialog);

// The one thing here that draws: a themed check box or radio button paints its
// own label through the theme and ignores the colour WM_CTLCOLORSTATIC hands
// back, so in dark mode the label comes out near-black on a near-black
// background. The theme still paints the glyph; only the label is ours.
//
// Call it from the dialog's WM_NOTIFY with the raw lParam. Returns true when it
// handled the notification, having written the reply to *result; false means
// the message was not ours and the caller should carry on.
bool DarkModeButtonCustomDraw(LPARAM lParam, LRESULT* result);
