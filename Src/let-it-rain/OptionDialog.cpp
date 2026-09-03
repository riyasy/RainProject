#include "OptionDialog.h"

#include <d2d1.h>

#include "DarkMode.h"
#include "Resource.h"
#include "SettingsManager.h"

OptionsDialog* OptionsDialog::pThis;
std::vector<CallBackWindow*> OptionsDialog::subscribers;

OptionsDialog::OptionsDialog(const HINSTANCE hInstance,
                             const int maxParticles,
                             const int windDirection,
                             const COLORREF particleColor,
                             const ParticleType partType,
                             const bool startWithWindows,
                             const bool allowHide,
                             const bool simpleSnowHeap)
	: hInstance(hInstance),
	  hDialog(nullptr),
	  MaxParticles(maxParticles),
	  WindDirection(windDirection),
	  ParticleColor(particleColor),
	  PartType(partType),
	  StartWithWindows(startWithWindows),
	  AllowHide(allowHide),
	  SimpleSnowHeap(simpleSnowHeap)
{
	pThis = this;
}

// Rain mode shows the Wind Direction controls; Snow mode replaces them with the
// "Simple snow heap" checkbox in the same spot.
static void SetSnowUiVisible(const HWND hWnd, const bool snow)
{
	const int windControls[] = {
		IDC_STATIC_WIND, IDC_SLIDER2, IDC_STATIC_WIND_LEFT, IDC_STATIC_WIND_RIGHT
	};
	for (const int id : windControls)
	{
		ShowWindow(GetDlgItem(hWnd, id), snow ? SW_HIDE : SW_SHOW);
	}
	ShowWindow(GetDlgItem(hWnd, IDC_CHECK_SIMPLE_SNOW), snow ? SW_SHOW : SW_HIDE);
}

void OptionsDialog::SubscribeToChange(CallBackWindow* subscriber)
{
	subscribers.push_back(subscriber);
}

bool OptionsDialog::Create()
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&icex);

	hDialog = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOGB), nullptr, reinterpret_cast<DLGPROC>(DialogProc), 0);
	if (!hDialog)
	{
		return false;
	}
	return true;
}

void OptionsDialog::Show() const
{
	ShowWindow(hDialog, SW_SHOW);
	SetForegroundWindow(hDialog);
}

void OptionsDialog::ApplyTheme() const
{
	if (hDialog) ThemeDialog(hDialog);
}

LRESULT CALLBACK OptionsDialog::DialogProc(const HWND hWnd, const UINT message, const WPARAM wParam,
                                           const LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			// Load and set the window icon (for both title bar and taskbar)
			HICON hIcon = LoadIcon(pThis->hInstance, MAKEINTRESOURCE(IDI_RAINCPPDXGI));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

			SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(5, 50));
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETRANGE, TRUE, MAKELONG(-5, 5));

			SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETPOS, TRUE, pThis->MaxParticles);
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETPOS, TRUE, pThis->WindDirection);

			if (pThis->PartType == RAIN)
			{
				SendMessage(GetDlgItem(hWnd, IDC_RADIO1), BM_SETCHECK, BST_CHECKED, 0);
			}
			else
			{
				SendMessage(GetDlgItem(hWnd, IDC_RADIO2), BM_SETCHECK, BST_CHECKED, 0);
			}

			// Initialize the simple-snow-heap checkbox and show the correct
			// control group for the current particle type.
			SendMessage(GetDlgItem(hWnd, IDC_CHECK_SIMPLE_SNOW), BM_SETCHECK,
				pThis->SimpleSnowHeap ? BST_CHECKED : BST_UNCHECKED, 0);
			SetSnowUiVisible(hWnd, pThis->PartType == SNOW);

			// Initialize startup checkbox
			SendMessage(GetDlgItem(hWnd, IDC_CHECK_STARTUP), BM_SETCHECK,
				pThis->StartWithWindows ? BST_CHECKED : BST_UNCHECKED, 0);

			// Initialize allow hide checkbox
			SendMessage(GetDlgItem(hWnd, IDC_CHECK_ALLOW_HIDE), BM_SETCHECK,
				pThis->AllowHide ? BST_CHECKED : BST_UNCHECKED, 0);

			// Same as the About box: no focus rectangle on whichever control the
			// dialog manager happens to focus, until the user reaches for Tab.
			SendMessage(hWnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);

			ThemeDialog(hWnd);
		}
		return TRUE;
	case WM_NOTIFY:
		{
			// Check boxes and radio buttons paint their own label and ignore the
			// colour WM_CTLCOLORSTATIC returns, so in dark mode they come here.
			LRESULT drawResult = 0;
			if (DarkModeButtonCustomDraw(lParam, &drawResult))
			{
				SetWindowLongPtr(hWnd, DWLP_MSGRESULT, drawResult);
				return TRUE;
			}
			return FALSE;
		}
	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER))
		{
			// TBM_GETPOS returns the position in an LRESULT; this trackbar's
			// range is 5..50, so narrowing to int cannot lose anything.
			const int pos = static_cast<int>(SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_GETPOS, 0, 0));
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateParticleCount(pos);
			}
		}
		else if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER2))
		{
			// Same again; this one's range is -5..5.
			const int pos = static_cast<int>(SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_GETPOS, 0, 0));
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateWindDirection(pos);
			}
		}
		return TRUE;
	case WM_COMMAND:
	{
		const int controlId = LOWORD(wParam);
		if (controlId == IDC_BUTTON_SHOW_COLOR)
		{
			CHOOSECOLOR cc;
			static COLORREF acrCustClr[16]; // array of custom colors 
			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = hWnd;
			cc.lpCustColors = static_cast<LPDWORD>(acrCustClr);
			cc.rgbResult = pThis->ParticleColor;
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;

			if (ChooseColor(&cc) == TRUE)
			{
				pThis->ParticleColor = cc.rgbResult;
				for (CallBackWindow* subscriber : subscribers)
				{
					subscriber->UpdateParticleColor(pThis->ParticleColor);
				}
			}
		}
		else if (controlId == IDC_RADIO1 && HIWORD(wParam) == BN_CLICKED)
		{
			SetSnowUiVisible(hWnd, false);
			pThis->PartType = RAIN;
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateParticleType(pThis->PartType);
			}
		}
		else if (controlId == IDC_RADIO2 && HIWORD(wParam) == BN_CLICKED)
		{
			SetSnowUiVisible(hWnd, true);
			pThis->PartType = SNOW;
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateParticleType(pThis->PartType);
			}
		}
		else if (controlId == IDC_CHECK_STARTUP && HIWORD(wParam) == BN_CLICKED)
		{
			const bool isChecked = SendMessage(GetDlgItem(hWnd, IDC_CHECK_STARTUP), BM_GETCHECK, 0, 0) == BST_CHECKED;
			pThis->StartWithWindows = isChecked;
			SettingsManager::SetStartupEnabled(isChecked);
		}
		else if (controlId == IDC_CHECK_ALLOW_HIDE && HIWORD(wParam) == BN_CLICKED)
		{
			const LRESULT checkState = SendMessage(GetDlgItem(hWnd, IDC_CHECK_ALLOW_HIDE), BM_GETCHECK, 0, 0);
			pThis->AllowHide = (checkState == BST_CHECKED);
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateAllowHide(pThis->AllowHide);
			}
		}
		else if (controlId == IDC_CHECK_SIMPLE_SNOW && HIWORD(wParam) == BN_CLICKED)
		{
			const LRESULT checkState = SendMessage(GetDlgItem(hWnd, IDC_CHECK_SIMPLE_SNOW), BM_GETCHECK, 0, 0);
			pThis->SimpleSnowHeap = (checkState == BST_CHECKED);
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateSnowHeapMode(pThis->SimpleSnowHeap);
			}
		}
		return TRUE;
	}
	// Windows themes a control's glyphs once ThemeDialog has named the dark
	// theme class, but it never paints the surface behind them. That is what
	// these do. Returning the brush handle from a dialog procedure is the
	// documented way to set it.
	case WM_CTLCOLORDLG:
		return reinterpret_cast<LRESULT>(ThemeBrush());
	case WM_CTLCOLORSTATIC: // labels, and the text beside a checkbox or radio
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
