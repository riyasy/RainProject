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
// MinClock's About box uses.
static const WCHAR* FLYPHOTOS_STORE =
	L"ms-windows-store://pdp/?productid=9PMSK128V1QT&cid=LetItRainAbout";
static const WCHAR* FLYPHOTOS_WEB =
	L"https://apps.microsoft.com/detail/9pmsk128v1qt?cid=LetItRainAbout&mode=full";
static const WCHAR* DESKTICK_STORE =
	L"ms-windows-store://pdp/?productid=9NQGFVNBX4WJ&cid=LetItRainAbout";
static const WCHAR* DESKTICK_WEB =
	L"https://apps.microsoft.com/detail/9nqgfvnbx4wj?cid=LetItRainAbout&mode=full";

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

AboutDialog::AboutDialog(const HINSTANCE hInstance)
	: hInstance(hInstance), hDialog(nullptr)
{
	pThis = this;
}

// The two labels this does not touch — title and version — come from version.h
// and are the same in every language. Everything else is keyed by its English
// text; see loc.h.
static void LocalizeDialog(const HWND hWnd)
{
	SetWindowText(hWnd, T(L"About - Let It Rain FX"));

	// Assembled from three pieces rather than drawn from VER_COPYRIGHT whole,
	// because only the last piece is prose. The sign and the holder are
	// identity: hand a translator "© RYF Tools. All rights reserved." as one
	// key and every one of them has to retype the holder inside their value,
	// where a typo is a wrong copyright notice — and rebranding would silently
	// drop all of the translations at once, the key having changed. So the
	// holder comes from VER_COMPANY untranslated and only the sentence goes
	// through T().
	//
	// ©, not the \xA9 version.h uses: this is a wide literal, and the
	// universal character name is the escape the compiler resolves whatever
	// the file's encoding.
	WCHAR copyright[160];
	if (SUCCEEDED(StringCchPrintfW(copyright, _countof(copyright), L"\u00A9 %s. %s",
	                               _CRT_WIDE(VER_COMPANY), T(L"All rights reserved."))))
	{
		SetDlgItemText(hWnd, IDC_ABOUT_COPYRIGHT, copyright);
	}

	SetDlgItemText(hWnd, IDC_ABOUT_FEEDBACK, T(L"Report issues or send feedback to"));
	SetDlgItemText(hWnd, IDC_ABOUT_OTHERAPPS, T(L"Other apps"));
	SetDlgItemText(hWnd, IDC_ABOUT_FLY_BLURB,
	               T(L"Fast, lightweight, and minimalist photo viewer designed for the modern Windows"));
	SetDlgItemText(hWnd, IDC_ABOUT_TICK_BLURB,
	               T(L"Customizable, minimal, transparent desktop clock widgets for Windows"));

	// The heart is an icon that happens to live in a label, not a word, so it
	// stays out of the translation file and is pasted back on here. No
	// translator has to carry an emoji plus its variation selector through an
	// .ini for it to survive.
	WCHAR sponsor[96];
	if (SUCCEEDED(StringCchPrintfW(sponsor, _countof(sponsor), L"\x2764\xFE0F %s", T(L"Support"))))
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
				SendMessage(GetDlgItem(hWnd, IDC_ABOUT_FLY_NAME), WM_SETFONT, (WPARAM)s_headFont, TRUE);
				SendMessage(GetDlgItem(hWnd, IDC_ABOUT_TICK_NAME), WM_SETFONT, (WPARAM)s_headFont, TRUE);
			}

			SetRowIcon(hWnd, IDC_ABOUT_FLY_ICON, IDI_FLYPHOTOS_ICON, pThis->hInstance);
			SetRowIcon(hWnd, IDC_ABOUT_TICK_ICON, IDI_DESKTICK_ICON, pThis->hInstance);

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
			const int id = GetDlgCtrlID(reinterpret_cast<HWND>(wParam));
			if (id == IDC_ABOUT_FLY_ICON || id == IDC_ABOUT_FLY_NAME
				|| id == IDC_ABOUT_TICK_ICON || id == IDC_ABOUT_TICK_NAME)
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
			else if ((controlId == IDC_ABOUT_FLY_ICON || controlId == IDC_ABOUT_FLY_NAME)
				&& HIWORD(wParam) == STN_CLICKED)
			{
				OpenStorePage(hWnd, FLYPHOTOS_STORE, FLYPHOTOS_WEB);
			}
			else if ((controlId == IDC_ABOUT_TICK_ICON || controlId == IDC_ABOUT_TICK_NAME)
				&& HIWORD(wParam) == STN_CLICKED)
			{
				OpenStorePage(hWnd, DESKTICK_STORE, DESKTICK_WEB);
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
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return TRUE;
	default: ;
	}
	return FALSE;
}
