#include "OptionDialog.h"

#include <d2d1.h>

#include "Resource.h"

OptionsDialog* OptionsDialog::pThis;
std::vector<CallBackWindow*> OptionsDialog::subscribers;

OptionsDialog::OptionsDialog(const HINSTANCE hInstance,
                             const int maxParticles,
                             const int windDirection,
                             const COLORREF particleColor,
                             const ParticleType partType)
	: hInstance(hInstance),
	  hDialog(nullptr),
	  MaxParticles(maxParticles),
	  WindDirection(windDirection),
	  ParticleColor(particleColor),
	  PartType(partType)
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

LRESULT CALLBACK OptionsDialog::DialogProc(const HWND hWnd, const UINT message, const WPARAM wParam,
                                           const LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
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
				SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), WM_ENABLE, FALSE, 0);
			}

			// Load the image from resources
			HICON hIcon = LoadIcon(pThis->hInstance, MAKEINTRESOURCE(IDI_GITHUB_ICON));

			// Get handle to the button
			const HWND hButton = GetDlgItem(hWnd, IDC_BUTTON_GITHUB);

			// Set the button style to allow both image and text
			const LONG_PTR style = GetWindowLongPtr(hButton, GWL_STYLE);
			//SetWindowLongPtr(hButton, GWL_STYLE, style | BS_ICON | BS_TEXT);
			SetWindowLongPtr(hButton, GWL_STYLE, style | BS_CENTER | BS_TEXT);

			// Set the image and the text
			SendMessage(hButton, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));
			SetWindowText(hButton, L"Github Repo");
		}
		return TRUE;
	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER))
		{
			const int pos = SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_GETPOS, 0, 0);
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateParticleCount(pos);
			}
		}
		else if (reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, IDC_SLIDER2))
		{
			const int pos = SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_GETPOS, 0, 0);
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
		else if (controlId == IDC_BUTTON_GITHUB)
		{
			ShellExecute(nullptr, L"open", L"https://github.com/riyasy/RainProject", nullptr, nullptr, SW_SHOWNORMAL);
		}
		else if (controlId == IDC_BUTTON_SPONSOR)
		{
			ShellExecute(nullptr, L"open", L"https://github.com/sponsors/riyasy", nullptr, nullptr, SW_SHOWNORMAL);
		}
		else if (controlId == IDC_RADIO1 && HIWORD(wParam) == BN_CLICKED)
		{
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), WM_ENABLE, TRUE, 0);
			pThis->PartType = RAIN;
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateParticleType(pThis->PartType);
			}
		}
		else if (controlId == IDC_RADIO2 && HIWORD(wParam) == BN_CLICKED)
		{
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), WM_ENABLE, FALSE, 0);
			pThis->PartType = SNOW;
			for (CallBackWindow* subscriber : subscribers)
			{
				subscriber->UpdateParticleType(pThis->PartType);
			}
		}
		return TRUE;
	}
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return TRUE;
	default: ;
	}
	return FALSE;
}
