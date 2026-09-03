#include "DarkMode.h"

#include <uxtheme.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <vsstyle.h>
#include <vssym32.h>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

// There is no public API for menu theming. Ordinals 135 (SetPreferredAppMode),
// 132 (ShouldAppsUseDarkMode), 104 (RefreshImmersiveColorPolicyState) and 136
// (FlushMenuThemes) are what Explorer itself uses.
//
// The build check in InitDarkMode is load-bearing, not politeness: before
// 17763 those ordinals are unrelated private functions with different
// signatures, so calling them would not merely fail, it would misbehave.
static void (WINAPI* g_refreshColorPolicy)();       // ordinal 104
static bool (WINAPI* g_shouldAppsUseDark)();        // ordinal 132
static void (WINAPI* g_flushMenuThemes)();          // ordinal 136

static HBRUSH g_darkBrush;

bool IsDarkMode()
{
	return g_shouldAppsUseDark && g_shouldAppsUseDark();
}

// Order matters, and it is the whole reason 104 is here: uxtheme caches the
// light/dark policy, so flushing the menu theme on its own re-resolves to the
// value cached at startup and the menu never changes.
void ReflushMenuTheme()
{
	if (g_refreshColorPolicy) g_refreshColorPolicy();
	if (g_flushMenuThemes) g_flushMenuThemes();
}

void InitDarkMode()
{
	OSVERSIONINFOW vi = {sizeof(vi)};               // RTL_OSVERSIONINFOW is the same layout
	LONG (WINAPI* getVer)(OSVERSIONINFOW*) = reinterpret_cast<LONG (WINAPI*)(OSVERSIONINFOW*)>(
		GetProcAddress(GetModuleHandle(L"ntdll"), "RtlGetVersion"));
	if (!getVer || getVer(&vi) != 0 || vi.dwBuildNumber < 17763)
	{
		return;                                     // pre-1809: no dark mode, leave it alone
	}

	const HMODULE ux = LoadLibrary(L"uxtheme.dll"); // never freed: process lifetime
	if (!ux) return;

	int (WINAPI* setAppMode)(int) = reinterpret_cast<int (WINAPI*)(int)>(
		GetProcAddress(ux, MAKEINTRESOURCEA(135)));
	g_refreshColorPolicy = reinterpret_cast<void (WINAPI*)()>(GetProcAddress(ux, MAKEINTRESOURCEA(104)));
	g_shouldAppsUseDark = reinterpret_cast<bool (WINAPI*)()>(GetProcAddress(ux, MAKEINTRESOURCEA(132)));
	g_flushMenuThemes = reinterpret_cast<void (WINAPI*)()>(GetProcAddress(ux, MAKEINTRESOURCEA(136)));

	if (setAppMode) setAppMode(1);                  // 1 = AllowDark, i.e. follow the system
	ReflushMenuTheme();
}

// The dark brush is created on first use and kept for the process lifetime.
HBRUSH ThemeBrush()
{
	if (!IsDarkMode()) return GetSysColorBrush(COLOR_3DFACE);
	if (!g_darkBrush) g_darkBrush = CreateSolidBrush(DARK_BG);
	return g_darkBrush;
}

void ThemeControl(const HWND control, const WCHAR* darkClass)
{
	SetWindowTheme(control, IsDarkMode() ? darkClass : nullptr, nullptr);
}

void ThemeCaption(const HWND window)
{
	BOOL dark = IsDarkMode();
	// 20 = DWMWA_USE_IMMERSIVE_DARK_MODE; it was 19 before 19H1, and an
	// unsupported attribute just fails, so try the modern one first.
	if (FAILED(DwmSetWindowAttribute(window, 20, &dark, sizeof(dark))))
	{
		DwmSetWindowAttribute(window, 19, &dark, sizeof(dark));
	}
}

void ThemeDialog(const HWND dialog)
{
	ThemeCaption(dialog);
	// Every control across both dialogs is a Button, a Static, a trackbar or a
	// SysLink — none of them a combo box or an edit, so one theme class covers
	// the lot and there is no need to branch on class name.
	//
	// Note the ceiling: msctls_trackbar32 has no dark variant in aero.msstyles.
	// The two sliders get a dark background from WM_CTLCOLORSTATIC but keep a
	// light channel and thumb. Owner-draw is the only way past that, and this
	// app has decided against owner-draw.
	EnumChildWindows(dialog, [](const HWND child, LPARAM) -> BOOL
	{
		ThemeControl(child, L"DarkMode_Explorer");
		return TRUE;
	}, 0);
	InvalidateRect(dialog, nullptr, TRUE);
}

bool DarkModeButtonCustomDraw(const LPARAM lParam, LRESULT* result)
{
	const NMHDR* hdr = reinterpret_cast<NMHDR*>(lParam);
	if (!hdr || hdr->code != NM_CUSTOMDRAW || !IsDarkMode()) return false;

	// Push buttons are left alone: DarkMode_Explorer already renders those
	// correctly, frame and caption both. Only the two styles that draw a label
	// beside a glyph need us, and BS_PUSHLIKE turns either of them back into a
	// push button.
	const LONG style = GetWindowLong(hdr->hwndFrom, GWL_STYLE);
	if (style & BS_PUSHLIKE) return false;
	const LONG type = style & BS_TYPEMASK;
	const bool isRadio = (type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON);
	const bool isCheck = (type == BS_AUTOCHECKBOX || type == BS_CHECKBOX);
	if (!isRadio && !isCheck) return false;

	const NMCUSTOMDRAW* cd = reinterpret_cast<NMCUSTOMDRAW*>(lParam);
	if (cd->dwDrawStage != CDDS_PREERASE)
	{
		*result = CDRF_DODEFAULT;
		return true;
	}

	// The control already carries the DarkMode_Explorer override from
	// ThemeControl, so this resolves to the dark button class where the running
	// Windows has one and the stock class where it does not.
	const HTHEME theme = OpenThemeData(hdr->hwndFrom, L"Button");
	if (!theme) return false;                       // let the default drawing have it

	// RBS_* and CBS_* share their numbering, so one ladder covers both parts.
	const int part = isRadio ? BP_RADIOBUTTON : BP_CHECKBOX;
	const bool checked = SendMessage(hdr->hwndFrom, BM_GETCHECK, 0, 0) == BST_CHECKED;
	int state;
	if (cd->uItemState & CDIS_DISABLED) state = checked ? CBS_CHECKEDDISABLED : CBS_UNCHECKEDDISABLED;
	else if (cd->uItemState & CDIS_SELECTED) state = checked ? CBS_CHECKEDPRESSED : CBS_UNCHECKEDPRESSED;
	else if (cd->uItemState & CDIS_HOT) state = checked ? CBS_CHECKEDHOT : CBS_UNCHECKEDHOT;
	else state = checked ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;

	FillRect(cd->hdc, &cd->rc, ThemeBrush());

	SIZE glyph = {};
	GetThemePartSize(theme, cd->hdc, part, state, nullptr, TS_DRAW, &glyph);
	RECT box = cd->rc;
	box.top += ((box.bottom - box.top) - glyph.cy) / 2;     // centre it on the row
	box.bottom = box.top + glyph.cy;
	box.right = box.left + glyph.cx;
	DrawThemeBackground(theme, cd->hdc, part, state, &box, nullptr);

	WCHAR label[256];
	const int len = GetWindowText(hdr->hwndFrom, label, ARRAYSIZE(label));
	RECT text = cd->rc;
	// The gap Windows itself leaves between glyph and label, scaled for DPI.
	text.left = box.right + MulDiv(6, GetDpiForWindow(hdr->hwndFrom), 96);
	SetBkMode(cd->hdc, TRANSPARENT);
	SetTextColor(cd->hdc, (cd->uItemState & CDIS_DISABLED) ? RGB(128, 128, 128) : DARK_FG);
	const HGDIOBJ oldFont = SelectObject(
		cd->hdc, reinterpret_cast<HFONT>(SendMessage(hdr->hwndFrom, WM_GETFONT, 0, 0)));
	DrawText(cd->hdc, label, len, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	// CDIS_FOCUS reports where focus actually is, not whether Windows wants
	// focus cues shown — so honour the UI state too, or a mouse-opened dialog
	// paints a ring the stock controls would have kept hidden.
	if ((cd->uItemState & CDIS_FOCUS) &&
		!(SendMessage(hdr->hwndFrom, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
	{
		RECT focus = text;
		DrawText(cd->hdc, label, len, &focus, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
		focus.top = text.top;
		focus.bottom = text.bottom;
		DrawFocusRect(cd->hdc, &focus);
	}
	SelectObject(cd->hdc, oldFont);
	CloseThemeData(theme);

	*result = CDRF_SKIPDEFAULT;
	return true;
}
