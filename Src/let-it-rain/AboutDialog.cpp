#include "AboutDialog.h"

#include <commctrl.h>
#include <shellapi.h>
#include <strsafe.h>

#include "DarkMode.h"
#include "loc.h"
#include "Resource.h"
#include "version.h"                     // shared with let-it-rain.rc — see there

#pragma comment(lib, "comctl32.lib")

AboutDialog* AboutDialog::pThis;

// Two URLs, and the order matters. ms-windows-store://pdp goes straight to the
// Store app; the apps.microsoft.com link is a web page, so the browser wins it
// and the Store only opens if the page decides to hand off. The scheme is not
// registered on every Windows (LTSC, Server, a stripped image), so the web page
// is the fallback for a click that would otherwise do nothing — never the first
// choice.
//
// cid is the campaign id Partner Center attributes installs to. Same value on
// both routes, and named for the surface it sits on, matching the convention
// DeskTick's About box uses.
//
// Every app lists the others in one order — FlyPhotos, Let It Rain, WTick,
// DeskTick, DeskFlip, less itself — with the same blurb wording, so keep this
// table in step with the other four apps' About boxes. The rows' controls are
// in let-it-rain.rc, in the same order.
struct OtherApp
{
	int iconCtl, nameCtl, blurbCtl; // the row's three controls; icon and name both click
	int iconRes;
	UINT blurb;
	const WCHAR* store; // ms-windows-store: opens the Store app
	const WCHAR* web;   // https: used only if that scheme is dead
};

static const OtherApp OTHER_APPS[] = {
	{ IDC_ABOUT_FLY_ICON, IDC_ABOUT_FLY_NAME, IDC_ABOUT_FLY_BLURB, IDI_FLYPHOTOS_ICON, IDS_FLYPHOTOS_BLURB,
	  L"ms-windows-store://pdp/?productid=9PMSK128V1QT&cid=LetItRainAbout",
	  L"https://apps.microsoft.com/detail/9pmsk128v1qt?cid=LetItRainAbout&mode=full" },
	{ IDC_ABOUT_WTICK_ICON, IDC_ABOUT_WTICK_NAME, IDC_ABOUT_WTICK_BLURB, IDI_WTICK_ICON, IDS_WTICK_BLURB,
	  L"ms-windows-store://pdp/?productid=9P039TR9SW9S&cid=LetItRainAbout",
	  L"https://apps.microsoft.com/detail/9p039tr9sw9s?cid=LetItRainAbout&mode=full" },
	{ IDC_ABOUT_TICK_ICON, IDC_ABOUT_TICK_NAME, IDC_ABOUT_TICK_BLURB, IDI_DESKTICK_ICON, IDS_DESKTICK_BLURB,
	  L"ms-windows-store://pdp/?productid=9NQGFVNBX4WJ&cid=LetItRainAbout",
	  L"https://apps.microsoft.com/detail/9nqgfvnbx4wj?cid=LetItRainAbout&mode=full" },
	{ IDC_ABOUT_FLIP_ICON, IDC_ABOUT_FLIP_NAME, IDC_ABOUT_FLIP_BLURB, IDI_DESKFLIP_ICON, IDS_DESKFLIP_BLURB,
	  L"ms-windows-store://pdp/?productid=9MSXBKV3295F&cid=LetItRainAbout",
	  L"https://apps.microsoft.com/detail/9msxbkv3295f?cid=LetItRainAbout&mode=full" },
};

static constexpr UINT WM_APP_LAYOUT = WM_APP + 1;

// The row whose icon or name has this control id, or null.
static const OtherApp* AppForControl(const int id)
{
	for (const OtherApp& app : OTHER_APPS)
	{
		if (id == app.iconCtl || id == app.nameCtl) return &app;
	}
	return nullptr;
}

// ShellExecute returns <= 32 when nothing claims the scheme, which is the only
// way to find out — so try the Store app first, then the web page.
static void OpenStorePage(const HWND hWnd, const WCHAR* store, const WCHAR* web)
{
	if (reinterpret_cast<INT_PTR>(ShellExecute(hWnd, L"open", store, nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
	{
		ShellExecute(hWnd, L"open", web, nullptr, nullptr, SW_SHOWNORMAL);
	}
}

// Derived from the dialog's own font rather than naming a family, so the box
// follows whatever Windows is set to — including a user's larger text. Process
// lifetime: the dialog is created once at startup and only hidden on close, so
// there is nothing to free them at.
static HFONT s_titleFont, s_headFont;

// Load at the control's own size rather than letting the static stretch a fixed
// frame, which it would do far more coarsely.
static void SetRowIcon(const HWND hWnd, const int controlId, const int iconId, const HINSTANCE hInstance)
{
	const HWND hControl = GetDlgItem(hWnd, controlId);
	RECT iconRect;
	GetClientRect(hControl, &iconRect);
	const HICON hIcon = static_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(iconId), IMAGE_ICON,
	                                                 iconRect.right, iconRect.bottom, LR_DEFAULTCOLOR));
	SendMessage(hControl, STM_SETICON, reinterpret_cast<WPARAM>(hIcon), 0);
}

// Control rect in the dialog's client coordinates.
static RECT ChildRect(const HWND hWnd, const HWND hChild)
{
	RECT r;
	GetWindowRect(hChild, &r);
	MapWindowPoints(nullptr, hWnd, reinterpret_cast<POINT*>(&r), 2);
	return r;
}

// Stack the "Other apps" rows under their heading, each blurb as tall as its
// text actually measures, and fit the dialog to the last one. The template's
// fixed 30-unit blurb boxes would clip a translation that wraps to a fourth
// line, and leave slack under every one that doesn't. DT_CALCRECT with the
// control's own font and width is what the STATIC itself will do, so the
// measurement and the drawing cannot disagree.
static void LayoutOtherApps(const HWND hWnd)
{
	RECT du = { 0, 0, 2, 8 }; // 2 DLUs between name and blurb, 8 between rows
	MapDialogRect(hWnd, &du);
	RECT margin = { 0, 0, 0, 10 }; // below the last row
	MapDialogRect(hWnd, &margin);

	int y = ChildRect(hWnd, GetDlgItem(hWnd, IDC_ABOUT_OTHERAPPS)).bottom + du.bottom;
	for (const OtherApp& app : OTHER_APPS)
	{
		const HWND hIcon = GetDlgItem(hWnd, app.iconCtl);
		const HWND hName = GetDlgItem(hWnd, app.nameCtl);
		const HWND hBlurb = GetDlgItem(hWnd, app.blurbCtl);
		const RECT ri = ChildRect(hWnd, hIcon);
		const RECT rn = ChildRect(hWnd, hName);
		const RECT rb = ChildRect(hWnd, hBlurb);

		WCHAR text[512];
		GetWindowText(hBlurb, text, _countof(text));
		const HDC dc = GetDC(hBlurb);
		const HGDIOBJ old = SelectObject(dc, reinterpret_cast<HFONT>(SendMessage(hBlurb, WM_GETFONT, 0, 0)));
		RECT m = { 0, 0, rb.right - rb.left, 0 };
		DrawText(dc, text, -1, &m, DT_CALCRECT | DT_WORDBREAK | DT_LEFT);
		SelectObject(dc, old);
		ReleaseDC(hBlurb, dc);

		const int nameH = rn.bottom - rn.top;
		const int blurbY = y + nameH + du.right;
		SetWindowPos(hIcon, nullptr, ri.left, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(hName, nullptr, rn.left, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(hBlurb, nullptr, rb.left, blurbY, rb.right - rb.left, m.bottom,
		             SWP_NOZORDER | SWP_NOACTIVATE);
		// At least as tall as the icon, so the next row clears it.
		y = max(blurbY + m.bottom, y + (ri.bottom - ri.top)) + du.bottom;
	}

	RECT client;
	GetClientRect(hWnd, &client);
	RECT wr = { 0, 0, client.right, y - du.bottom + margin.bottom };
	AdjustWindowRectExForDpi(&wr, static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE)), FALSE,
	                         static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_EXSTYLE)), GetDpiForWindow(hWnd));
	SetWindowPos(hWnd, nullptr, 0, 0, wr.right - wr.left, wr.bottom - wr.top,
	             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

AboutDialog::AboutDialog(const HINSTANCE hInstance)
	: hInstance(hInstance), hDialog(nullptr)
{
	pThis = this;
}

// The two labels this does not touch — title and version — come from version.h
// and are the same in every language. Everything else comes from
// Localization\translations.csv; see loc.h.
static void LocalizeDialog(const HWND hWnd)
{
	SetWindowText(hWnd, T(IDS_ABOUT_TITLE));

	// Assembled from three pieces rather than drawn from VER_COPYRIGHT whole,
	// because only the last piece is prose. The sign and the holder are
	// identity: hand a translator "© RYF Tools. All rights reserved." as one
	// key and every one of them has to retype the holder inside their value,
	// where a typo is a wrong copyright notice, and a rebrand means editing
	// all nineteen. So the holder comes from VER_COMPANY untranslated and only
	// the sentence goes through T().
	//
	// ©, not the \xA9 version.h uses: this is a wide literal, and the
	// universal character name is the escape the compiler resolves whatever
	// the file's encoding.
	WCHAR copyright[160];
	if (SUCCEEDED(StringCchPrintfW(copyright, _countof(copyright), L"\u00A9 %s. %s",
	                               _CRT_WIDE(VER_COMPANY), T(IDS_RIGHTS_RESERVED))))
	{
		SetDlgItemText(hWnd, IDC_ABOUT_COPYRIGHT, copyright);
	}

	SetDlgItemText(hWnd, IDC_ABOUT_FEEDBACK, T(IDS_FEEDBACK));
	SetDlgItemText(hWnd, IDC_ABOUT_OTHERAPPS, T(IDS_OTHER_APPS));
	for (const OtherApp& app : OTHER_APPS)
	{
		SetDlgItemText(hWnd, app.blurbCtl, T(app.blurb));
	}

	// The heart is an icon that happens to live in a label, not a word, so it
	// stays out of the translation file and is pasted back on here. No
	// translator has to carry an emoji plus its variation selector through a
	// spreadsheet for it to survive.
	WCHAR sponsor[96];
	if (SUCCEEDED(StringCchPrintfW(sponsor, _countof(sponsor), L"\x2764\xFE0F %s", T(IDS_SUPPORT))))
	{
		SetDlgItemText(hWnd, IDC_BUTTON_SPONSOR, sponsor);
	}
}

bool AboutDialog::Create()
{
	// The SysLink on this dialog is the only comctl32 class asked for by name;
	// the rest are user32's, redirected to v6 by app.manifest.
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LINK_CLASS;
	InitCommonControlsEx(&icex);

	hDialog = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_ABOUT), nullptr,
	                            reinterpret_cast<DLGPROC>(DialogProc), 0);
	return hDialog != nullptr;
}

void AboutDialog::Show() const
{
	ShowWindow(hDialog, SW_SHOW);
	SetForegroundWindow(hDialog);
}

void AboutDialog::ApplyTheme() const
{
	if (hDialog) ThemeDialog(hDialog);
}

LRESULT CALLBACK AboutDialog::DialogProc(const HWND hWnd, const UINT message, const WPARAM wParam,
                                         const LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			// Before the fonts below measure anything.
			LocalizeDialog(hWnd);

			const HICON hIcon = LoadIcon(pThis->hInstance, MAKEINTRESOURCE(IDI_RAINCPPDXGI));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));

			// A larger semibold title and a semibold heading, both derived from
			// the dialog's own font.
			LOGFONT lf = {};
			const HFONT dialogFont = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
			if (dialogFont && GetObject(dialogFont, sizeof(lf), &lf))
			{
				LOGFONT big = lf;
				big.lfHeight = lf.lfHeight * 9 / 5;
				big.lfWeight = FW_SEMIBOLD;
				s_titleFont = CreateFontIndirect(&big);
				LOGFONT mid = lf;
				mid.lfWeight = FW_SEMIBOLD;
				s_headFont = CreateFontIndirect(&mid);
				SendMessage(GetDlgItem(hWnd, IDC_ABOUT_TITLE), WM_SETFONT, (WPARAM)s_titleFont, TRUE);
				SendMessage(GetDlgItem(hWnd, IDC_ABOUT_OTHERAPPS), WM_SETFONT, (WPARAM)s_headFont, TRUE);
				for (const OtherApp& app : OTHER_APPS)
				{
					SendMessage(GetDlgItem(hWnd, app.nameCtl), WM_SETFONT, (WPARAM)s_headFont, TRUE);
				}
			}

			for (const OtherApp& app : OTHER_APPS)
			{
				SetRowIcon(hWnd, app.iconCtl, app.iconRes, pThis->hInstance);
			}
			LayoutOtherApps(hWnd);

			// Github icon button
			const HICON hGitHubIcon = static_cast<HICON>(LoadImage(
				pThis->hInstance, MAKEINTRESOURCE(IDI_GITHUB_ICON), IMAGE_ICON, 24, 24,
				LR_DEFAULTCOLOR));
			SendMessage(GetDlgItem(hWnd, IDC_BUTTON_GITHUB), BM_SETIMAGE, IMAGE_ICON,
			            reinterpret_cast<LPARAM>(hGitHubIcon));

			// No focus cues until the user actually navigates by keyboard, which
			// is what Windows does for a mouse-opened dialog anyway. Tab still
			// brings them back.
			SendMessage(hWnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);

			ThemeDialog(hWnd);

			// The SysLink is the first WS_TABSTOP control, so the dialog manager
			// would otherwise open the box with a mailto link focused. Returning
			// FALSE says we placed the focus ourselves.
			SetFocus(GetDlgItem(hWnd, IDC_BUTTON_GITHUB));
		}
		return FALSE;
	case WM_NOTIFY:
		{
			const NMHDR* hdr = reinterpret_cast<NMHDR*>(lParam);
			if (hdr->idFrom == IDC_ABOUT_MAIL_LINK && (hdr->code == NM_CLICK || hdr->code == NM_RETURN))
			{
				// The href lives in the control, so nothing here knows the address.
				ShellExecute(hWnd, L"open", reinterpret_cast<const NMLINK*>(lParam)->item.szUrl,
				             nullptr, nullptr, SW_SHOWNORMAL);
				return TRUE;
			}
			return FALSE;
		}
	case WM_SETCURSOR:
		{
			// A child's WM_SETCURSOR reaches us through its DefWindowProc, so the
			// hand cursor for both halves of each "Other apps" row is one handler
			// rather than a subclass each.
			if (AppForControl(GetDlgCtrlID(reinterpret_cast<HWND>(wParam))))
			{
				SetCursor(LoadCursor(nullptr, IDC_HAND));
				SetWindowLongPtr(hWnd, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
			return FALSE;
		}
	case WM_COMMAND:
		{
			const int controlId = LOWORD(wParam);
			if (controlId == IDC_BUTTON_GITHUB)
			{
				ShellExecute(nullptr, L"open", L"https://github.com/riyasy/RainProject",
				             nullptr, nullptr, SW_SHOWNORMAL);
			}
			else if (controlId == IDC_BUTTON_SPONSOR)
			{
				ShellExecute(nullptr, L"open", L"https://github.com/sponsors/riyasy",
				             nullptr, nullptr, SW_SHOWNORMAL);
			}
			// The icon and the name open the same page.
			else if (const OtherApp* app = AppForControl(controlId); app && HIWORD(wParam) == STN_CLICKED)
			{
				OpenStorePage(hWnd, app->store, app->web);
			}
			else if (controlId == IDCANCEL)         // Escape
			{
				ShowWindow(hWnd, SW_HIDE);
			}
			return TRUE;
		}
	// Windows themes a control's glyphs once ThemeDialog has named the dark
	// theme class, but it never paints the surface behind them. That is what
	// these do. Returning the brush handle from a dialog procedure is the
	// documented way to set it.
	case WM_CTLCOLORDLG:
		return reinterpret_cast<LRESULT>(ThemeBrush());
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:
		SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
		// GetSysColor rather than a literal in light mode, so high contrast
		// themes still come out readable.
		SetTextColor(reinterpret_cast<HDC>(wParam),
		             IsDarkMode() ? DARK_FG : GetSysColor(COLOR_WINDOWTEXT));
		return reinterpret_cast<LRESULT>(ThemeBrush());
	// Windows rescales a PerMonitorV2 dialog itself when it changes monitor, so
	// let that run and re-stack the rows once it has.
	case WM_DPICHANGED:
		PostMessage(hWnd, WM_APP_LAYOUT, 0, 0);
		return FALSE;
	case WM_APP_LAYOUT:
		LayoutOtherApps(hWnd);
		return TRUE;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return TRUE;
	default: ;
	}
	return FALSE;
}
