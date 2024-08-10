#include "OptionDialog.h"

#include <d2d1.h>

#include "Resource.h"

OptionsDialog* OptionsDialog::pThis;

OptionsDialog::OptionsDialog(HINSTANCE hInstance, CallBackWindow* pRainWindow, int maxRainDrops,
                             int rainDirection,
                             COLORREF rainColor) : hInstance(hInstance),
                                                   hDialog(nullptr),
                                                   pRainWindow(pRainWindow),
                                                   MaxRainDrops(maxRainDrops),
                                                   RainDirection(rainDirection),
                                                   RainColor(rainColor)
{
	pThis = this;
}

bool OptionsDialog::Create()
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&icex);

	hDialog = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOGB), nullptr, DialogProc, 0);
	if (!hDialog)
	{
		return false;
	}
	return true;
}

void OptionsDialog::Show() const
{
	ShowWindow(hDialog, SW_SHOW);
}

LRESULT CALLBACK OptionsDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(5, 50));
		SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETRANGE, TRUE, MAKELONG(-5, 5));

		SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETPOS, TRUE, pThis->MaxRainDrops);
		SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETPOS, TRUE, pThis->RainDirection);

		return TRUE;
	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER))
		{
			int pos = SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_GETPOS, 0, 0);
			pThis->pRainWindow->UpdateRainDropCount(pos);
		}
		else if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER2))
		{
			int pos = SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_GETPOS, 0, 0);
			pThis->pRainWindow->UpdateRainDirection(pos);
		}
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON_SHOW_COLOR)
		{
			//ShellExecute(NULL, L"open", L"https://www.google.com", NULL, NULL, SW_SHOWNORMAL);

			CHOOSECOLOR cc;
			static COLORREF acrCustClr[16]; // array of custom colors 
			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = hWnd;
			cc.lpCustColors = static_cast<LPDWORD>(acrCustClr);
			cc.rgbResult = pThis->RainColor;
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;

			if (ChooseColor(&cc) == TRUE)
			{
				pThis->RainColor = cc.rgbResult;
				pThis->pRainWindow->UpdateRainColor(pThis->RainColor);
			}

			return TRUE;
		}
		break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return TRUE;
	default: ;
	}
	return FALSE;
}
