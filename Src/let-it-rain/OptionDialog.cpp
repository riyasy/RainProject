#include "OptionDialog.h"

#include <d2d1.h>
#include <memory>
#include <string_view>

#include "Resource.h"

// Initialize static members
OptionsDialog* OptionsDialog::pThis{nullptr};
std::vector<CallBackWindow*> OptionsDialog::subscribers{};

OptionsDialog::OptionsDialog(const HINSTANCE hInstance,
                             const int maxParticles,
                             const int windDirection,
                             const COLORREF particleColor,
                             const ParticleType partType,
                             const int lightningFreq,
                             const int lightningIntensity)
	: hInstance{hInstance},
	  hDialog{nullptr},
	  MaxParticles{maxParticles},
	  WindDirection{windDirection},
	  ParticleColor{particleColor},
	  PartType{partType},
	  LightningFrequency{lightningFreq},
	  LightningIntensity{lightningIntensity}
{
	pThis = this;
}

void OptionsDialog::SubscribeToChange(CallBackWindow* subscriber)
{
    if (subscriber != nullptr) {
	    subscribers.push_back(subscriber);
    }
}

bool OptionsDialog::Create()
{
	// Initialize common controls
	INITCOMMONCONTROLSEX icex{};
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_BAR_CLASSES;
	if (!InitCommonControlsEx(&icex)) {
		return false;
	}

	// Create the dialog
	hDialog = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOGB), nullptr, DialogProc, 0);
	if (!hDialog) {
		return false;
	}
	
	// Center the dialog on the primary monitor
	CenterDialog();
	
	return true;
}

void OptionsDialog::CenterDialog() const
{
    // Get primary monitor dimensions
    constexpr std::wstring_view windowClassName = L"window";
    constexpr std::wstring_view windowTitle = L"let it rain";
    const HWND mainWindow = FindWindow(windowClassName.data(), windowTitle.data());
    
    RECT dialogRect{};
    GetWindowRect(hDialog, &dialogRect);
    const int dialogWidth = dialogRect.right - dialogRect.left;
    const int dialogHeight = dialogRect.bottom - dialogRect.top;
    
    if (mainWindow != nullptr) {
        // Center based on main window
        RECT mainWindowRect{};
        if (GetWindowRect(mainWindow, &mainWindowRect)) {
            const int mainWindowWidth = mainWindowRect.right - mainWindowRect.left;
            const int mainWindowHeight = mainWindowRect.bottom - mainWindowRect.top;
            const int x = mainWindowRect.left + (mainWindowWidth - dialogWidth) / 2;
            const int y = mainWindowRect.top + (mainWindowHeight - dialogHeight) / 2;
            
            // Set window position
            SetWindowPos(hDialog, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            return;
        }
    }
    
    // Fallback to screen center if main window not found or cannot get its rect
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    const int x = (screenWidth - dialogWidth) / 2;
    const int y = (screenHeight - dialogHeight) / 2;
    
    SetWindowPos(hDialog, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void OptionsDialog::Show() const
{
	if (hDialog) {
	    ShowWindow(hDialog, SW_SHOW);
	}
}

LRESULT CALLBACK OptionsDialog::DialogProc(const HWND hWnd, const UINT message, const WPARAM wParam,
                                           const LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			 // Setup sliders
			// Particles slider (5-250)
			constexpr int PARTICLE_MIN = 5;
			constexpr int PARTICLE_MAX = 250;
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(PARTICLE_MIN, PARTICLE_MAX));
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER), TBM_SETPOS, TRUE, pThis->MaxParticles);
			
			// Wind direction slider (-5 to 5)
			constexpr int WIND_MIN = -5;
			constexpr int WIND_MAX = 5;
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETRANGE, TRUE, MAKELONG(WIND_MIN, WIND_MAX));
			SendMessage(GetDlgItem(hWnd, IDC_SLIDER2), TBM_SETPOS, TRUE, pThis->WindDirection);

			// Lightning frequency slider (0-100) - control should exist in RC file
			HWND hLightningFreqSlider = GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_FREQ);
			if (hLightningFreqSlider)
			{
				constexpr int LIGHTNING_FREQ_MIN = 0;
				constexpr int LIGHTNING_FREQ_MAX = 100;
				SendMessage(hLightningFreqSlider, TBM_SETRANGE, TRUE, MAKELONG(LIGHTNING_FREQ_MIN, LIGHTNING_FREQ_MAX));
				SendMessage(hLightningFreqSlider, TBM_SETPOS, TRUE, pThis->LightningFrequency);
			}

			// Lightning intensity slider (0-100) - control should exist in RC file
			HWND hLightningIntensitySlider = GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_INTENSITY);
			if (hLightningIntensitySlider)
			{
				constexpr int LIGHTNING_INTENSITY_MIN = 0;
				constexpr int LIGHTNING_INTENSITY_MAX = 100;
				SendMessage(hLightningIntensitySlider, TBM_SETRANGE, TRUE, MAKELONG(LIGHTNING_INTENSITY_MIN, LIGHTNING_INTENSITY_MAX));
				SendMessage(hLightningIntensitySlider, TBM_SETPOS, TRUE, pThis->LightningIntensity);
			}

			// Set initial radio button state based on particle type
			if (pThis->PartType == RAIN)
			{
				SendMessage(GetDlgItem(hWnd, IDC_RADIO1), BM_SETCHECK, BST_CHECKED, 0);
				// Enable lightning controls for rain
				if (hLightningFreqSlider) EnableWindow(hLightningFreqSlider, TRUE);
				if (hLightningIntensitySlider) EnableWindow(hLightningIntensitySlider, TRUE);
			}
			else
			{
				SendMessage(GetDlgItem(hWnd, IDC_RADIO2), BM_SETCHECK, BST_CHECKED, 0);
				// Disable wind direction and lightning sliders for snow
				EnableWindow(GetDlgItem(hWnd, IDC_SLIDER2), FALSE);
				if (hLightningFreqSlider) EnableWindow(hLightningFreqSlider, FALSE);
				if (hLightningIntensitySlider) EnableWindow(hLightningIntensitySlider, FALSE);
			}

			// Set up GitHub button with icon
			const HICON hIcon = LoadIcon(pThis->hInstance, MAKEINTRESOURCE(IDI_GITHUB_ICON));
			if (hIcon)
			{
				const HWND hButton = GetDlgItem(hWnd, IDC_BUTTON_GITHUB);
				if (hButton)
				{
					// Set the button style to center text
					const LONG_PTR style = GetWindowLongPtr(hButton, GWL_STYLE);
					SetWindowLongPtr(hButton, GWL_STYLE, style | BS_CENTER | BS_TEXT);

					// Set the icon and text
					SendMessage(hButton, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));
					SetWindowText(hButton, L"GitHub Repo");
				}
			}
		}
		return TRUE;

	case WM_HSCROLL:
		{
			const HWND hControl = reinterpret_cast<HWND>(lParam);
			
			// Check which slider was moved
			if (hControl == GetDlgItem(hWnd, IDC_SLIDER))
			{
				// Particle count slider
				const int pos = static_cast<int>(SendMessage(hControl, TBM_GETPOS, 0, 0));
				NotifyParticleCountChange(pos);
			}
			else if (hControl == GetDlgItem(hWnd, IDC_SLIDER2))
			{
				// Wind direction slider
				const int pos = static_cast<int>(SendMessage(hControl, TBM_GETPOS, 0, 0));
				NotifyWindDirectionChange(pos);
			}
			else if (hControl == GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_FREQ))
			{
				// Lightning frequency slider
				const int pos = static_cast<int>(SendMessage(hControl, TBM_GETPOS, 0, 0));
				NotifyLightningFrequencyChange(pos);
			}
			else if (hControl == GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_INTENSITY))
			{
				// Lightning intensity slider
				const int pos = static_cast<int>(SendMessage(hControl, TBM_GETPOS, 0, 0));
				NotifyLightningIntensityChange(pos);
			}
		}
		return TRUE;

	case WM_COMMAND:
		{
			const int controlId = LOWORD(wParam);
			const int notificationCode = HIWORD(wParam);

			switch (controlId)
			{
			case IDC_BUTTON_SHOW_COLOR:
				ShowColorChooserDialog(hWnd);
				break;
				
			case IDC_BUTTON_GITHUB:
				// Open GitHub repository in browser
				ShellExecute(nullptr, L"open", L"https://github.com/riyasy/RainProject", 
					nullptr, nullptr, SW_SHOWNORMAL);
				break;
				
			case IDC_RADIO1:
				if (notificationCode == BN_CLICKED)
				{
					// Enable wind direction and lightning sliders for rain
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER2), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_FREQ), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_INTENSITY), TRUE);
					pThis->PartType = RAIN;
					NotifyParticleTypeChange(pThis->PartType);
				}
				break;
				
			case IDC_RADIO2:
				if (notificationCode == BN_CLICKED)
				{
					// Disable wind direction and lightning sliders for snow
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER2), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_FREQ), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_INTENSITY), FALSE);
					pThis->PartType = SNOW;
					NotifyParticleTypeChange(pThis->PartType);
				}
				break;
			}
		}
		return TRUE;

	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return TRUE;

	default:
		return FALSE;
	}
}

void OptionsDialog::ShowColorChooserDialog(const HWND hWnd)
{
	static COLORREF acrCustClr[16] = {}; // Array of custom colors (preserve between calls)
	
	CHOOSECOLOR cc{};
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = static_cast<LPDWORD>(acrCustClr);
	cc.rgbResult = pThis->ParticleColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor(&cc))
	{
		pThis->ParticleColor = cc.rgbResult;
		NotifyParticleColorChange(pThis->ParticleColor);
	}
}

void OptionsDialog::NotifyParticleCountChange(const int newValue)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateParticleCount(newValue);
		}
	}
}

void OptionsDialog::NotifyWindDirectionChange(const int newValue)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateWindDirection(newValue);
		}
	}
}

void OptionsDialog::NotifyParticleColorChange(const COLORREF newColor)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateParticleColor(newColor);
		}
	}
}

void OptionsDialog::NotifyParticleTypeChange(const ParticleType newType)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateParticleType(newType);
		}
	}
}

void OptionsDialog::NotifyLightningFrequencyChange(const int newValue)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateLightningFrequency(newValue);
		}
	}
}

void OptionsDialog::NotifyLightningIntensityChange(const int newValue)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateLightningIntensity(newValue);
		}
	}
}