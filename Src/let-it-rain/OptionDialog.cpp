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
                             const int lightningIntensity,
                             const bool enableSnowWind,
                             const int snowWindIntensity,
                             const int snowWindVariability)
	: hInstance{hInstance},
	  hDialog{nullptr},
	  MaxParticles{maxParticles},
	  WindDirection{windDirection},
	  ParticleColor{particleColor},
	  PartType{partType},
	  LightningFrequency{lightningFreq},
	  LightningIntensity{lightningIntensity},
	  EnableSnowWind{enableSnowWind},
	  SnowWindIntensity{snowWindIntensity},
	  SnowWindVariability{snowWindVariability}
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

void OptionsDialog::UpdateSnowWindControlsState(const HWND hWnd, const bool enabled)
{
	// Enable/disable snow wind sliders based on the checkbox state
	EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_INTENSITY), enabled);
	EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_VARIABILITY), enabled);
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

			// Lightning frequency slider (0-100)
			HWND hLightningFreqSlider = GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_FREQ);
			if (hLightningFreqSlider)
			{
				constexpr int LIGHTNING_FREQ_MIN = 0;
				constexpr int LIGHTNING_FREQ_MAX = 100;
				SendMessage(hLightningFreqSlider, TBM_SETRANGE, TRUE, MAKELONG(LIGHTNING_FREQ_MIN, LIGHTNING_FREQ_MAX));
				SendMessage(hLightningFreqSlider, TBM_SETPOS, TRUE, pThis->LightningFrequency);
			}

			// Lightning intensity slider (0-100)
			HWND hLightningIntensitySlider = GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_INTENSITY);
			if (hLightningIntensitySlider)
			{
				constexpr int LIGHTNING_INTENSITY_MIN = 0;
				constexpr int LIGHTNING_INTENSITY_MAX = 100;
				SendMessage(hLightningIntensitySlider, TBM_SETRANGE, TRUE, MAKELONG(LIGHTNING_INTENSITY_MIN, LIGHTNING_INTENSITY_MAX));
				SendMessage(hLightningIntensitySlider, TBM_SETPOS, TRUE, pThis->LightningIntensity);
			}
			
			// Enable Snow Wind checkbox
			HWND hEnableSnowWindCheck = GetDlgItem(hWnd, IDC_CHECK_SNOW_WIND);
			if (hEnableSnowWindCheck)
			{
				SendMessage(hEnableSnowWindCheck, BM_SETCHECK, pThis->EnableSnowWind ? BST_CHECKED : BST_UNCHECKED, 0);
			}
			
			// Snow wind intensity slider (0-100)
			HWND hSnowWindIntensitySlider = GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_INTENSITY);
			if (hSnowWindIntensitySlider)
			{
				constexpr int SNOW_WIND_INTENSITY_MIN = 0;
				constexpr int SNOW_WIND_INTENSITY_MAX = 100;
				SendMessage(hSnowWindIntensitySlider, TBM_SETRANGE, TRUE, MAKELONG(SNOW_WIND_INTENSITY_MIN, SNOW_WIND_INTENSITY_MAX));
				SendMessage(hSnowWindIntensitySlider, TBM_SETPOS, TRUE, pThis->SnowWindIntensity);
			}
			
			// Snow wind variability slider (0-100)
			HWND hSnowWindVariabilitySlider = GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_VARIABILITY);
			if (hSnowWindVariabilitySlider)
			{
				constexpr int SNOW_WIND_VARIABILITY_MIN = 0;
				constexpr int SNOW_WIND_VARIABILITY_MAX = 100;
				SendMessage(hSnowWindVariabilitySlider, TBM_SETRANGE, TRUE, MAKELONG(SNOW_WIND_VARIABILITY_MIN, SNOW_WIND_VARIABILITY_MAX));
				SendMessage(hSnowWindVariabilitySlider, TBM_SETPOS, TRUE, pThis->SnowWindVariability);
			}

			// Set initial radio button state based on particle type
			if (pThis->PartType == RAIN)
			{
				SendMessage(GetDlgItem(hWnd, IDC_RADIO1), BM_SETCHECK, BST_CHECKED, 0);
				// Enable lightning controls for rain
				if (hLightningFreqSlider) EnableWindow(hLightningFreqSlider, TRUE);
				if (hLightningIntensitySlider) EnableWindow(hLightningIntensitySlider, TRUE);
				
				// Disable snow wind controls since we're in rain mode
				if (hEnableSnowWindCheck) EnableWindow(hEnableSnowWindCheck, FALSE);
				if (hSnowWindIntensitySlider) EnableWindow(hSnowWindIntensitySlider, FALSE);
				if (hSnowWindVariabilitySlider) EnableWindow(hSnowWindVariabilitySlider, FALSE);
			}
			else
			{
				SendMessage(GetDlgItem(hWnd, IDC_RADIO2), BM_SETCHECK, BST_CHECKED, 0);
				// Disable wind direction and lightning sliders for snow
				EnableWindow(GetDlgItem(hWnd, IDC_SLIDER2), FALSE);
				if (hLightningFreqSlider) EnableWindow(hLightningFreqSlider, FALSE);
				if (hLightningIntensitySlider) EnableWindow(hLightningIntensitySlider, FALSE);
				
				// Enable snow wind checkbox control
				if (hEnableSnowWindCheck) EnableWindow(hEnableSnowWindCheck, TRUE);
				
				// Set snow wind controls state based on the checkbox
				UpdateSnowWindControlsState(hWnd, pThis->EnableSnowWind);
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
			else if (hControl == GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_INTENSITY))
			{
				// Snow wind intensity slider
				const int pos = static_cast<int>(SendMessage(hControl, TBM_GETPOS, 0, 0));
				NotifySnowWindIntensity(pos);
			}
			else if (hControl == GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_VARIABILITY))
			{
				// Snow wind variability slider
				const int pos = static_cast<int>(SendMessage(hControl, TBM_GETPOS, 0, 0));
				NotifySnowWindVariability(pos);
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
				ShellExecute(nullptr, L"open", L"https://github.com/todddube/RainProject", 
					nullptr, nullptr, SW_SHOWNORMAL);
				break;
				
			case IDC_RADIO1:
				if (notificationCode == BN_CLICKED)
				{
					// Enable wind direction and lightning sliders for rain
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER2), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_FREQ), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_LIGHTNING_INTENSITY), TRUE);
					
					// Disable snow wind settings for rain
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_SNOW_WIND), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_INTENSITY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_SNOW_WIND_VARIABILITY), FALSE);
					
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
					
					// Enable snow wind checkbox
					EnableWindow(GetDlgItem(hWnd, IDC_CHECK_SNOW_WIND), TRUE);
					
					// Set snow wind controls state based on the checkbox
					bool enableSnowWind = (SendMessage(GetDlgItem(hWnd, IDC_CHECK_SNOW_WIND), BM_GETCHECK, 0, 0) == BST_CHECKED);
					UpdateSnowWindControlsState(hWnd, enableSnowWind);
					
					pThis->PartType = SNOW;
					NotifyParticleTypeChange(pThis->PartType);
				}
				break;
				
			case IDC_CHECK_SNOW_WIND:
				if (notificationCode == BN_CLICKED)
				{
					// Snow wind checkbox was clicked
					bool enabled = (SendMessage(GetDlgItem(hWnd, IDC_CHECK_SNOW_WIND), BM_GETCHECK, 0, 0) == BST_CHECKED);
					pThis->EnableSnowWind = enabled;
					
					// Enable/disable snow wind sliders based on checkbox state
					UpdateSnowWindControlsState(hWnd, enabled);
					
					// Notify callback
					NotifyEnableSnowWind(enabled);
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

void OptionsDialog::NotifyEnableSnowWind(const bool enabled)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateEnableSnowWind(enabled);
		}
	}
}

void OptionsDialog::NotifySnowWindIntensity(const int newValue)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateSnowWindIntensity(newValue);
		}
	}
}

void OptionsDialog::NotifySnowWindVariability(const int newValue)
{
	for (auto* subscriber : subscribers)
	{
		if (subscriber != nullptr) {
			subscriber->UpdateSnowWindVariability(newValue);
		}
	}
}