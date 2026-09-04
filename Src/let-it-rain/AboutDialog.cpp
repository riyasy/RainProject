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

// Derived from the dialog's own font rather than naming a family, so the box
// follows whatever Windows is set to — including a user's larger text. Process
// lifetime: the dialog is created once at startup and only hidden on close, so
// there is nothing to free them at.
static HFONT s_titleFont, s_headFont;

AboutDialog::AboutDialog(const HINSTANCE hInstance)
	: hInstance(hInstance), hDialog(nullptr)
{
	pThis = this;
}

// The three labels this does not touch — title, version and copyright — come
// from version.h and are the same in every language. Everything else is keyed
// by its English text; see loc.h.
static void LocalizeDialog(const HWND hWnd)
{
	SetWindowText(hWnd, T(L"About - Let It Rain FX"));
	SetDlgItemText(hWnd, IDC_ABOUT_FEEDBACK, T(L"Report issues or send feedback to"));
	SetDlgItemText(hWnd, IDC_ABOUT_OTHERAPPS, T(L"Other apps"));
	SetDlgItemText(hWnd, IDC_ABOUT_FLY_BLURB,
	               T(L"Fast, lightweight, and minimalist photo viewer designed for the modern Windows"));

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
			}

			// Load at the control's own size rather than letting the static
			// stretch a fixed frame, which it would do far more coarsely.
			RECT iconRect;
			GetClientRect(GetDlgItem(hWnd, IDC_ABOUT_FLY_ICON), &iconRect);
			const HICON hFlyIcon = static_cast<HICON>(LoadImage(
				pThis->hInstance, MAKEINTRESOURCE(IDI_FLYPHOTOS_ICON), IMAGE_ICON,
				iconRect.right, iconRect.bottom, LR_DEFAULTCOLOR));
			SendMessage(GetDlgItem(hWnd, IDC_ABOUT_FLY_ICON), STM_SETICON,
			            reinterpret_cast<WPARAM>(hFlyIcon), 0);

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
			// hand cursor for both halves of the FlyPhotos row is one handler
			// rather than a subclass each.
			const int id = GetDlgCtrlID(reinterpret_cast<HWND>(wParam));
			if (id == IDC_ABOUT_FLY_ICON || id == IDC_ABOUT_FLY_NAME)
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
			else if ((controlId == IDC_ABOUT_FLY_ICON || controlId == IDC_ABOUT_FLY_NAME)
				&& HIWORD(wParam) == STN_CLICKED)
			{
				// The icon and the name open the same page. ShellExecute returns
				// <= 32 when nothing claims the scheme, which is the only way to
				// find out — so try the Store app first, then the web page.
				if (reinterpret_cast<INT_PTR>(ShellExecute(hWnd, L"open", FLYPHOTOS_STORE,
				                                           nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
				{
					ShellExecute(hWnd, L"open", FLYPHOTOS_WEB, nullptr, nullptr, SW_SHOWNORMAL);
				}
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
