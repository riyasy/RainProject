#include "OptionDialog.h"

#include <d2d1.h>

#include "Resource.h"

OptionsDialog* OptionsDialog::pThis;
std::vector<CallBackWindow*> OptionsDialog::subscribers;

OptionsDialog::OptionsDialog(HINSTANCE hInstance, int maxRainDrops,
                             int rainDirection,
                             COLORREF rainColor) : hInstance(hInstance),
                                                   hDialog(nullptr),                                                   
                                                   MaxRainDrops(maxRainDrops),
                                                   RainDirection(rainDirection),
                                                   RainColor(rainColor)
{
	pThis = this;
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
		{
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(5, 50));
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETRANGE, TRUE, MAKELONG(-5, 5));

			SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETPOS, TRUE, pThis->MaxRainDrops);
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETPOS, TRUE, pThis->RainDirection);


			// Load the image from resources
			HICON hIcon = (HICON)LoadIcon(pThis->hInstance, MAKEINTRESOURCE(IDI_GITHUB_ICON));

			// Get handle to the button
			const HWND hButton = GetDlgItem(hWnd, IDC_BUTTON_GITHUB);

			// Set the button style to allow both image and text
			const LONG_PTR style = GetWindowLongPtr(hButton, GWL_STYLE);
			//SetWindowLongPtr(hButton, GWL_STYLE, style | BS_ICON | BS_TEXT);
			SetWindowLongPtr(hButton, GWL_STYLE, style |   BS_CENTER | BS_TEXT);

			// Set the image and the text
			SendMessage(hButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
			SetWindowText(hButton, L"Github Repo");

		}
		return TRUE;
	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER))
		{
			const int pos = SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_GETPOS, 0, 0);
			for (CallBackWindow* subscriber : subscribers) {
				subscriber->UpdateRainDropCount(pos);
			}
		}
		else if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER2))
		{
			const int pos = SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_GETPOS, 0, 0);
			for (CallBackWindow* subscriber : subscribers) {
				subscriber->UpdateRainDirection(pos);
			}
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
				for (CallBackWindow* subscriber : subscribers) {
					subscriber->UpdateRainColor(pThis->RainColor);
				}
			}
		}
		else if (LOWORD(wParam) == IDC_BUTTON_GITHUB) {
			ShellExecute(NULL, L"open", L"https://github.com/riyasy/RainProject", NULL, NULL, SW_SHOWNORMAL);
		}
		return TRUE;
		break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return TRUE;
	default: ;
	}
	return FALSE;
}
