#include "DisplayWindow.h"

#include <chrono>
#include <cstdlib>
#include <shellapi.h>
#include <commctrl.h>
#include <sstream>
#include <cmath>

#include "CPUUsageTracker.h"
#include "Global.h"
#include "MathUtil.h"
#include "Resource.h"
#include "SettingsManager.h"
#include "SnowFlake.h"
#include "RandomGenerator.h"
#include "FastNoiseLite.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#define NOTIFICATION_TRAY_ICON_UID 786

struct ComException
{
	HRESULT Result;

	ComException(const HRESULT value) :
		Result(value)
	{
	}
};

void HR(const HRESULT result)
{
	if (S_OK != result)
	{
		throw ComException(result);
	}
}

enum TIMERS
{
	DELAY_TIMER = 1947,
	INTERVAL_TIMER = 1948,
	Z_ORDER_TIMER = 1949
};

HINSTANCE DisplayWindow::AppInstance = nullptr;
OptionsDialog* DisplayWindow::pOptionsDlg;
Setting DisplayWindow::GeneralSettings;

HRESULT DisplayWindow::Initialize(const HINSTANCE hInstance, const MonitorData& monitorData)
{
	MonitorDat = monitorData;
	AppInstance = hInstance;
	WNDCLASS wc = {};
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = HINST_THISCOMPONENT;
	wc.lpszClassName = L"window";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	RegisterClass(&wc);

	// Remove WS_EX_TOPMOST flag to allow the window to go behind other windows
	// Keeping WS_EX_TRANSPARENT, WS_EX_LAYERED, and WS_EX_TOOLWINDOW
	constexpr DWORD exstyle = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW;
	// DWORD exstyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST;
	constexpr DWORD style = WS_POPUP | WS_VISIBLE;

	// Calculate window dimensions
	const int windowWidth = monitorData.MonitorRect.right - monitorData.MonitorRect.left;
	const int windowHeight = monitorData.MonitorRect.bottom - monitorData.MonitorRect.top;

	// Calculate centered position
	const int xPos = monitorData.MonitorRect.left + ((windowWidth - windowWidth) / 2);
	const int yPos = monitorData.MonitorRect.top + ((windowHeight - windowHeight) / 2);

	// center windows with dimensions
	const HWND window = CreateWindowEx(exstyle, wc.lpszClassName, L"let it rain", style,
		xPos,
		yPos,
		windowWidth,
		windowHeight,
		nullptr, nullptr, HINST_THISCOMPONENT, this);

	/* // Commented out to center window for the above 
		const HWND window = CreateWindowEx(exstyle, wc.lpszClassName, L"let it rain", style,
	                                   monitorData.MonitorRect.left,
	                                   monitorData.MonitorRect.top,
	                                   monitorData.MonitorRect.right - monitorData.MonitorRect.left,
	                                   monitorData.MonitorRect.bottom - monitorData.MonitorRect.top,
	                                   nullptr, nullptr, HINST_THISCOMPONENT, this);
	*/
	::ShowWindow(window, SW_SHOW);

	if (!GeneralSettings.loaded)
	{
		SettingsManager::GetInstance()->ReadSettings(GeneralSettings);
	}

	OptionsDialog::SubscribeToChange(this);
	if (MonitorDat.IsPrimaryDisplay)
	{
		pOptionsDlg = new OptionsDialog(AppInstance, GeneralSettings.MaxParticles, GeneralSettings.WindSpeed,
		                                GeneralSettings.ParticleColor, GeneralSettings.PartType,
		                                GeneralSettings.LightningFrequency, GeneralSettings.LightningIntensity,
		                                GeneralSettings.EnableSnowWind, GeneralSettings.SnowWindIntensity,
		                                GeneralSettings.SnowWindVariability);
		pOptionsDlg->Create();
	}

	InitDirect2D(window);
	pDisplaySpecificData = new DisplayData(Dc.Get());
	pDisplaySpecificData->SetRainColor(GeneralSettings.ParticleColor);
	HandleWindowBoundsChange(window, false);
	
	// Initialize snow wind system with default values
	CurrentSnowWindDirection = 0.0f;
	TargetSnowWindDirection = 0.0f;
	WindTransitionProgress = 1.0f;
	LastWindChangeTime = GetCurrentTimeInSeconds();
	NextWindChangeTime = LastWindChangeTime + 5.0; // First change in 5 seconds

	// Set the window to the bottom of the Z-order so it appears behind other windows
	SetWindowPos(window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	return 0;
}

void DisplayWindow::UpdateParticleCount(const int val)
{
	GeneralSettings.MaxParticles = val;
}

void DisplayWindow::UpdateWindDirection(const int val)
{
	GeneralSettings.WindSpeed = val;
}

void DisplayWindow::UpdateParticleColor(const COLORREF color)
{
	GeneralSettings.ParticleColor = color;
	pDisplaySpecificData->SetRainColor(color);
}

void DisplayWindow::UpdateParticleType(const ParticleType partType)
{
	GeneralSettings.PartType = partType;
}

void DisplayWindow::UpdateLightningFrequency(const int val)
{
	GeneralSettings.LightningFrequency = val;
}

void DisplayWindow::UpdateLightningIntensity(const int val)
{
	GeneralSettings.LightningIntensity = val;
}

void DisplayWindow::UpdateEnableSnowWind(const bool enabled)
{
	GeneralSettings.EnableSnowWind = enabled;
}

void DisplayWindow::UpdateSnowWindIntensity(const int val)
{
	GeneralSettings.SnowWindIntensity = val;
}

void DisplayWindow::UpdateSnowWindVariability(const int val)
{
	GeneralSettings.SnowWindVariability = val;
}

LRESULT DisplayWindow::WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
	static UINT_PTR timerId = 0;

	switch (message)
	{
	case WM_NCCREATE:
		{
			SetInstanceToHwnd(hWnd, lParam);
			break;
		}
	case WM_CREATE:
		{
			const DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			if (pThis->MonitorDat.IsPrimaryDisplay)
			{
				InitNotifyIcon(hWnd);
			}
			SetTimer(hWnd, INTERVAL_TIMER, 500, nullptr);
			
			 // Set up a timer for z-order management
			SetTimer(hWnd, Z_ORDER_TIMER, 100, nullptr);
			
			// Initial z-order positioning
			UpdateZOrder(hWnd);
			break;
		}
	case WM_DESTROY:
		{
			const DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			if (pThis->MonitorDat.IsPrimaryDisplay)
			{
				// saving settings in config ini file
				SettingsManager::GetInstance()->WriteSettings(GeneralSettings);
				RemoveNotifyIcon(hWnd);
			}
			PostQuitMessage(0);
			return 0;
		}
	case WM_DISPLAYCHANGE:
	case WM_DPICHANGED:
		{
			if (timerId != 0)
			{
				KillTimer(hWnd, timerId);
			}
			timerId = SetTimer(hWnd, DELAY_TIMER, 1000, nullptr);
			
			 // Update z-order after display changes
			UpdateZOrder(hWnd);
			break;
		}
	case WM_TIMER:
		if (wParam == DELAY_TIMER)
		{
			DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			pThis->HandleWindowBoundsChange(hWnd, true);
			KillTimer(hWnd, DELAY_TIMER);
			timerId = 0;
			
			 // Update z-order after window bounds change
			UpdateZOrder(hWnd);
		}
		if (wParam == INTERVAL_TIMER)
		{
			DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			pThis->HandleTaskBarChange();
		}
		if (wParam == Z_ORDER_TIMER)
		{
			// Regularly check and update z-order position
			UpdateZOrder(hWnd);
		}
		break;
	case WM_TRAYICON:
		if (lParam == WM_CONTEXTMENU)
		{
			ShowContextMenu(hWnd);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_TRAY_EXIT_CONTEXT_MENU_ITEM:
			DestroyWindow(hWnd);
			break;
		case ID_TRAY_CONFIGURE_CONTEXT_MENU_ITEM:
			pOptionsDlg->Show();
			break;
		default: ;
		}
		break;
	case WM_NCHITTEST:
		{
			LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
			if (hit == HTCLIENT) hit = HTCAPTION;
			return hit;
		}
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		// saving settings in config ini file
		SettingsManager::GetInstance()->WriteSettings(GeneralSettings);
		return TRUE;
	case WM_SHOWWINDOW:
		if (wParam == TRUE) // Window is being shown
		{
			// Update z-order when window is shown
			UpdateZOrder(hWnd);
		}
		break;
	case WM_WINDOWPOSCHANGED:
		{
			// Update z-order after position changes
			UpdateZOrder(hWnd);
			break;
		}
	case WM_KEYDOWN:
	case WM_PAINT:
		{
			break;
		}
	default: ;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

double DisplayWindow::GetCurrentTimeInSeconds()
{
	using namespace std::chrono;
	return duration<double>(high_resolution_clock::now().time_since_epoch()).count();
}

void DisplayWindow::Animate()
{
	// Target a stable physics time step of 1/120th of a second (more precise than previous 0.01)
	constexpr double fixedTimeStep = 1.0 / 120.0;

	if (CurrentTime < 0)
	{
		CurrentTime = GetCurrentTimeInSeconds();
		return; // Skip the first frame to establish timing
	}
	
	// Calculate actual frame time
	const double newTime = GetCurrentTimeInSeconds();
	double frameTime = newTime - CurrentTime;
	CurrentTime = newTime;
	
	// Cap maximum frame time to avoid "spiral of death" with very long frames
	if (frameTime > 0.25)
		frameTime = 0.25;
		
	// Add frame time to the accumulator
	Accumulator += frameTime;

	// Update with a fixed time step for physics stability
	int stepCount = 0;
	while (Accumulator >= fixedTimeStep && stepCount < 3) // Limit max steps per frame
	{
		// Update particle systems with fixed time step
		if (GeneralSettings.PartType == RAIN)
		{
			UpdateRainDrops(fixedTimeStep);
		}
		else if (GeneralSettings.PartType == SNOW)
		{
			// Update snow wind system if enabled
			if (GeneralSettings.EnableSnowWind)
			{
				UpdateSnowWind(fixedTimeStep);
			}
			
			UpdateSnowFlakes(fixedTimeStep);
		}
		
		// Update lightning flash system
		UpdateLightning();
		
		// Consume accumulated time
		Accumulator -= fixedTimeStep;
		stepCount++;
	}
	
	// Draw the current state
	if (GeneralSettings.PartType == RAIN)
	{
		DrawRainDrops();
	}
	else if (GeneralSettings.PartType == SNOW)
	{
		DrawSnowFlakes();
	}	
}

void DisplayWindow::UpdateSnowWind(const float deltaTime)
{
	const double currentTime = GetCurrentTimeInSeconds();
	
	// Check if it's time to change the wind direction
	if (currentTime >= NextWindChangeTime)
	{
		// Calculate how often the wind changes based on variability (0-100)
		// Higher variability means more frequent changes (2-15 seconds)
		// Reduced minimum time slightly to make wind changes more noticeable
		const float variabilityFactor = GeneralSettings.SnowWindVariability / 100.0f;
		const float changeDuration = 15.0f - (variabilityFactor * 13.0f);  // 2-15 seconds
		
		// Calculate how strong the wind is based on intensity (0-100)
		// Higher intensity means stronger wind (-8 to 8) - increased range for visibility
		// Increased the strength range from 4.0 to 8.0 to make wind more noticeable
		const float intensityFactor = GeneralSettings.SnowWindIntensity / 100.0f;
		const float maxWindStrength = 8.0f * intensityFactor;
		
		// Set the previous and target wind directions
		CurrentSnowWindDirection = TargetSnowWindDirection;
		
		// Generate a new target wind direction with more dramatic shifts
		auto& rng = RandomGenerator::GetInstance();
		
		// Make wind changes more dramatic by having more variance between directions
		// Reduced continuity factor to allow more dramatic shifts
		const float windContinuityFactor = 0.1f;  // Reduced from 0.3 for more dramatic shifts
		
		// Make wind shifts more pronounced by sometimes forcing direction change
		// If the current wind is weak or we need a dramatic change
		if (fabsf(CurrentSnowWindDirection) < 1.0f || rng.GenerateFloat(0.0f, 1.0f) < 0.3f) {
			// Create more dramatic wind shift
			const float randomComponent = rng.GenerateFloat(-maxWindStrength, maxWindStrength);
			// Force wind to blow in a more noticeable direction, avoiding near-zero values
			if (fabsf(randomComponent) < 2.0f) {
				TargetSnowWindDirection = randomComponent > 0 ? 2.0f : -2.0f;
			} else {
				TargetSnowWindDirection = randomComponent;
			}
		}
		else {
			// Normal wind shift with some continuity
			const float randomComponent = rng.GenerateFloat(-maxWindStrength, maxWindStrength);
			TargetSnowWindDirection = (CurrentSnowWindDirection * windContinuityFactor) + 
									  (randomComponent * (1.0f - windContinuityFactor));
		}
		
		// Cap the target wind strength
		if (TargetSnowWindDirection > maxWindStrength)
			TargetSnowWindDirection = maxWindStrength;
		if (TargetSnowWindDirection < -maxWindStrength)
			TargetSnowWindDirection = -maxWindStrength;
		
		// Reset transition progress
		WindTransitionProgress = 0.0f;
		
		// Schedule next wind change with some randomness for natural variation
		LastWindChangeTime = currentTime;
		NextWindChangeTime = currentTime + changeDuration + (rng.GenerateFloat(-2.0f, 2.0f));
	}
	
	// Update wind transition - slightly faster transitions for more noticeable effect
	if (WindTransitionProgress < 1.0f)
	{
		// Transition more quickly for more dramatic wind changes
		// Increased transition speed for more visible changes
		float transitionSpeed = 0.3f + 
		                       (GeneralSettings.SnowWindVariability / 100.0f * 0.4f);
		
		WindTransitionProgress += deltaTime * transitionSpeed;
		if (WindTransitionProgress > 1.0f)
		{
			WindTransitionProgress = 1.0f;
		}
	}
}

float DisplayWindow::GetCurrentSnowWindFactor() const
{
	if (!GeneralSettings.EnableSnowWind)
	{
		return 0.0f;
	}
	
	// Use smooth transition between wind directions
	const float t = WindTransitionProgress;
	
	// Modified easing function for more dramatic initial and final movements
	// This makes wind changes more noticeable at start and end of transition
	float easedT;
	if (t < 0.5f) {
		// Sharper initial acceleration (more noticeable start)
		easedT = 2.0f * t * t;
	}
	else {
		// Sharper deceleration at the end (more noticeable finish)
		const float f = (1.0f - t);
		easedT = 1.0f - 2.0f * f * f;
	}
	
	// Interpolate between current and target wind directions
	return CurrentSnowWindDirection * (1.0f - easedT) + TargetSnowWindDirection * easedT;
}

void DisplayWindow::InitNotifyIcon(const HWND hWnd)
{
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.hWnd = hWnd;

	// NIF_GUID and nid.guidItem doesn't work properly in Windows 10.
	// So used .uID parameter without NIF_GUID flag.
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP; // | NIF_GUID;
	//nid.guidItem = __uuidof(RainIcon); 
	nid.uID = NOTIFICATION_TRAY_ICON_UID;

	nid.uCallbackMessage = WM_TRAYICON;
	LoadIconMetric(AppInstance, MAKEINTRESOURCE(IDI_SMALL), LIM_SMALL, &nid.hIcon);
	Shell_NotifyIcon(NIM_ADD, &nid);

	// Context menu not working for NOTIFYICON_VERSION_4. So used NOTIFYICON_VERSION
	nid.uVersion = NOTIFYICON_VERSION; 
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void DisplayWindow::RemoveNotifyIcon(const HWND hWnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hWnd;
	nid.uID = NOTIFICATION_TRAY_ICON_UID;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void DisplayWindow::ShowContextMenu(const HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	const HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_TRAY_CONFIGURE_CONTEXT_MENU_ITEM, L"Configure");
	AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, L"Exit");
	SetForegroundWindow(hWnd);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
	DestroyMenu(hMenu);
}

void DisplayWindow::InitDirect2D(const HWND hWnd)
{
	HR(D3D11CreateDevice(nullptr, // Adapter
	                     D3D_DRIVER_TYPE_HARDWARE,
	                     nullptr, // Module
	                     D3D11_CREATE_DEVICE_BGRA_SUPPORT,
	                     nullptr, 0, // Highest available feature level
	                     D3D11_SDK_VERSION,
	                     Direct3dDevice.GetAddressOf(),
	                     nullptr, // Actual feature level
	                     nullptr)); // Device context

	HR(Direct3dDevice.As(&DxgiDevice));

	HR(CreateDXGIFactory2(
		0,
		__uuidof(DxFactory),
		reinterpret_cast<void**>(DxFactory.GetAddressOf())));

	DXGI_SWAP_CHAIN_DESC1 description = {};
	description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	description.BufferCount = 2;
	description.SampleDesc.Count = 1;
	description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

	RECT rect = {};
	GetClientRect(hWnd, &rect);
	description.Width = rect.right - rect.left;
	description.Height = rect.bottom - rect.top;

	HR(DxFactory->CreateSwapChainForComposition(DxgiDevice.Get(),
	                                            &description,
	                                            nullptr,
	                                            SwapChain.GetAddressOf()));

	// Create a single-threaded Direct2D factory with debugging information
	D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
	                     options,
	                     D2Factory.GetAddressOf()));

	// Create the Direct2D device that links back to the Direct3D device
	HR(D2Factory->CreateDevice(DxgiDevice.Get(),
	                           D2Device.GetAddressOf()));
	// Create the Direct2D device context that is the actual render target
	// and exposes drawing commands
	HR(D2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
	                                 Dc.GetAddressOf()));
	// Retrieve the swap chain's back buffer
	HR(SwapChain->GetBuffer(
		0, // index
		__uuidof(Surface),
		reinterpret_cast<void**>(Surface.GetAddressOf())));
	// Create a Direct2D bitmap that points to the swap chain surface
	D2D1_BITMAP_PROPERTIES1 properties = {};
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET |
		D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	HR(Dc->CreateBitmapFromDxgiSurface(Surface.Get(),
	                                   properties,
	                                   Bitmap.GetAddressOf()));

	// Point the device context to the bitmap for rendering
	Dc->SetTarget(Bitmap.Get());

	HR(DCompositionCreateDevice(
		DxgiDevice.Get(),
		__uuidof(DcompDevice),
		reinterpret_cast<void**>(DcompDevice.GetAddressOf())));

	// Set topmost to false to allow the window to appear behind other windows
	HR(DcompDevice->CreateTargetForHwnd(hWnd,
	                                    false, // Set to false to not be topmost
	                                    Target.GetAddressOf()));

	HR(DcompDevice->CreateVisual(Visual.GetAddressOf()));
	HR(Visual->SetContent(SwapChain.Get()));
	HR(Target->SetRoot(Visual.Get()));
	HR(DcompDevice->Commit());
}

void DisplayWindow::HandleWindowBoundsChange(const HWND window, const bool clearDrops)
{
	RECT sceneRect;
	float scaleFactor = 1.0f;
	// find screen rect which removes the taskbar at the bottom
	FindSceneRect(sceneRect, scaleFactor);
	if (sceneRect != pDisplaySpecificData->SceneRect)
	{
		if (!RainDrops.empty() && clearDrops)
		{
			RainDrops.clear();
		}
		pDisplaySpecificData->SetSceneBounds(sceneRect, scaleFactor);

		std::wostringstream  oss;
		oss << "Monitor Name: " << MonitorDat.Name.c_str() << ", "
			<< "SceneRect Bounds: "
			<< "Left: " << sceneRect.left << ", "
			<< "Top: " << sceneRect.top << ", "
			<< "Right: " << sceneRect.right << ", "
			<< "Bottom: " << sceneRect.bottom << "\n";;
		std::wstring logMessage = oss.str();
		OutputDebugStringW(logMessage.c_str());
	}
}

void DisplayWindow::HandleTaskBarChange() const
{
	RECT sceneRect;
	float scaleFactor = 1.0f;
	FindSceneRect(sceneRect, scaleFactor);
	if (sceneRect != pDisplaySpecificData->SceneRect)
	{
		pDisplaySpecificData->SetSceneBounds(sceneRect, scaleFactor);

		std::wostringstream  oss;
		oss << "Monitor Name: " << MonitorDat.Name.c_str() << ", "
			<< "SceneRect Bounds: "
			<< "Left: " << sceneRect.left << ", "
			<< "Top: " << sceneRect.top << ", "
			<< "Right: " << sceneRect.right << ", "
			<< "Bottom: " << sceneRect.bottom << "\n";
		std::wstring logMessage = oss.str();
		OutputDebugStringW(logMessage.c_str());
	}
}

void DisplayWindow::FindSceneRect(RECT& sceneRect, float& scaleFactor) const
{
	std::vector<MonitorData> monitorDataList;
	EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitorDataList));
	for (const auto& monitorData : monitorDataList)
	{
		if (monitorData.Name == MonitorDat.Name)
		{
			HWND hTaskbarWnd = nullptr;
			if (MonitorDat.IsPrimaryDisplay)
			{
				hTaskbarWnd = FindWindow(L"Shell_traywnd", nullptr);
			}
			else
			{
				//hTaskbarWnd = FindWindow(L"Shell_SecondaryTrayWnd", nullptr);

				// Task bar name is same for all the non primary monitors. So we need to enumerate and find the correct one
				// for each monitor.
				while ((hTaskbarWnd = FindWindowEx(nullptr, hTaskbarWnd, L"Shell_SecondaryTrayWnd", nullptr)) != nullptr)
				{
					const HMONITOR hMonitor = MonitorFromWindow(hTaskbarWnd, MONITOR_DEFAULTTONEAREST);
					MONITORINFOEX monitorInfo;
					monitorInfo.cbSize = sizeof(monitorInfo);
					if (GetMonitorInfo(hMonitor, &monitorInfo) && monitorInfo.szDevice == MonitorDat.Name)
					{
						break;
					}
				}
			}

			RECT taskBarRect;
			GetWindowRect(hTaskbarWnd, &taskBarRect);

			RECT desktopRect = monitorData.MonitorRect;

			const int monitorHeight = desktopRect.bottom - desktopRect.top;
			scaleFactor = static_cast<float>(monitorHeight) / 1080.0f;

			// Check if task bar is hidden and act accordingly
			if ((taskBarRect.top >= desktopRect.bottom - 4) ||
				(taskBarRect.right <= desktopRect.left + 2) ||
				(taskBarRect.bottom <= desktopRect.top + 4) ||
				(taskBarRect.left >= desktopRect.right - 2))
			{
				sceneRect = desktopRect;
			}
			else
			{
				sceneRect = MathUtil::SubtractRect(desktopRect, taskBarRect);
				if (sceneRect.left - sceneRect.right == 0) // in case of any error
				{
					sceneRect = desktopRect;
				}
			}

			sceneRect = MathUtil::NormalizeRect(sceneRect, desktopRect.top, desktopRect.left);
			break;
		}
	}
}

void DisplayWindow::FindSceneRect2(RECT& sceneRect, float& scaleFactor) const
{
	HWND hTaskbarWnd;
	if (MonitorDat.IsPrimaryDisplay)
	{
		hTaskbarWnd = FindWindow(L"Shell_traywnd", nullptr);
	}
	else
	{
		hTaskbarWnd = FindWindow(L"Shell_SecondaryTrayWnd", nullptr);
	}

	const HMONITOR hMonitor = MonitorFromWindow(hTaskbarWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info = {sizeof(MONITORINFO)};

	RECT taskBarRect, desktopRect;

	GetWindowRect(hTaskbarWnd, &taskBarRect);
	if (GetMonitorInfo(hMonitor, &info))
	{
		desktopRect = info.rcMonitor;
	}
	else
	{
		GetWindowRect(GetDesktopWindow(), &desktopRect);
	}

	const int top = desktopRect.top;
	const int left = desktopRect.left;

	// Check if task bar is hidden and act accordingly
	if ((taskBarRect.top >= desktopRect.bottom - 4) ||
		(taskBarRect.right <= desktopRect.left + 2) ||
		(taskBarRect.bottom <= desktopRect.top + 4) ||
		(taskBarRect.left >= desktopRect.right - 2))
	{
		sceneRect = desktopRect;
	}
	else
	{
		sceneRect = MathUtil::SubtractRect(desktopRect, taskBarRect);
		if (sceneRect.left - sceneRect.right == 0) // in case of any error
		{
			sceneRect = desktopRect;
		}
	}	

	sceneRect = MathUtil::NormalizeRect(sceneRect, top, left);

	const int monitorHeight = info.rcMonitor.bottom - info.rcMonitor.top;
	scaleFactor = static_cast<float>(monitorHeight) / 1080.0f;
}

void DisplayWindow::DrawRainDrops() const
{
	Dc->BeginDraw();
	Dc->Clear();

	// Draw lightning flash effect first (background layer)
	DrawLightningFlash();

	for (const auto pDrop : RainDrops)
	{
		pDrop->Draw(Dc.Get());
	}

	HR(Dc->EndDraw());
	// Make the swap chain available to the composition engine
	HR(SwapChain->Present(1, 0));
}

void DisplayWindow::DrawSnowFlakes() const
{
	Dc->BeginDraw();
	Dc->Clear();

	// Draw lightning flash effect first (background layer)
	DrawLightningFlash();

	for (const auto pFlake : SnowFlakes)
	{
		pFlake->Draw(Dc.Get());
	}

	if (!SnowFlakes.empty())
	{
		// SnowFlake::DrawSettledSnow(Dc.Get(), pDisplaySpecificData);
		SnowFlake::DrawSettledSnow2(Dc.Get(), pDisplaySpecificData);
	}

	HR(Dc->EndDraw());
	// Make the swap chain available to the composition engine
	HR(SwapChain->Present(1, 0));
}

void DisplayWindow::UpdateRainDrops(const float deltaTime)
{
	// Move each raindrop to the next point
	for (RainDrop* const pDrop : RainDrops)
	{
		pDrop->UpdatePosition(deltaTime); // Use proper delta time
	}

	// Remove all raindrops that have expired
	for (auto pDropIterator = RainDrops.begin(); pDropIterator != RainDrops.end();)
	{
		if ((*pDropIterator)->IsReadyForErase())
		{
			delete *pDropIterator;
			pDropIterator = RainDrops.erase(pDropIterator);
		}
		else
		{
			++pDropIterator;
		}
	}

	// Calculate the number of raindrops to generate
	int countOfFallingDrops = 0;
	for (const RainDrop* const pDrop : RainDrops)
	{
		if (!pDrop->DidTouchGround())
		{
			countOfFallingDrops++;
		}
	}

	const int noOfDropsToGenerate = GeneralSettings.MaxParticles * 3 - countOfFallingDrops;

	// Generate new raindrops
	for (int i = 0; i < noOfDropsToGenerate; ++i)
	{
		RainDrop* pDrop = new RainDrop(GeneralSettings.WindSpeed, pDisplaySpecificData);
		RainDrops.push_back(pDrop);
	}
}

void DisplayWindow::UpdateSnowFlakes(const float deltaTime)
{
	// Added 12/25/2024 - Todd D
	// rate of snow fall *100 added
	const int noOfFlakesToGenerate = GeneralSettings.MaxParticles * 100 - SnowFlakes.size();

	if (noOfFlakesToGenerate > 0)
	{
		for (int i = 0; i < noOfFlakesToGenerate; i++)
		{
			SnowFlake* pFlake = new SnowFlake(pDisplaySpecificData);
			SnowFlakes.push_back(pFlake);
		}
	}

	if (noOfFlakesToGenerate < 0)
	{
		const int noOfFlakesToErase = -noOfFlakesToGenerate;
		for (int i = 0; i < noOfFlakesToErase; i++)
		{
			delete SnowFlakes[i];
		}
		// Remove the first n elements
		SnowFlakes.erase(SnowFlakes.begin(), SnowFlakes.begin() + noOfFlakesToErase);
	}

	// Get current wind direction for snow (if enabled)
	const float snowWindFactor = GetCurrentSnowWindFactor();
	
	// Move each snowflake to the next point
	for (SnowFlake* const pFlake : SnowFlakes)
	{
		// Apply wind to horizontal velocity if snow wind is enabled
		if (GeneralSettings.EnableSnowWind && snowWindFactor != 0.0f)
		{
			// Add wind effect to the snowflake's velocity
			pFlake->ApplyWind(snowWindFactor, deltaTime);
		}
		
		pFlake->UpdatePosition(deltaTime); // Use proper delta time instead of hard-coded 0.01f
	}
	SnowFlake::SettleSnow(pDisplaySpecificData);
}

void DisplayWindow::SetInstanceToHwnd(const HWND hWnd, const LPARAM lParam)
{
	DisplayWindow* pThis = static_cast<DisplayWindow*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
}

DisplayWindow* DisplayWindow::GetInstanceFromHwnd(const HWND hWnd)
{
	return reinterpret_cast<DisplayWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

DisplayWindow::~DisplayWindow()
{
	delete pDisplaySpecificData;
}

void DisplayWindow::UpdateLightning()
{
	// Only enable lightning during rain, not snow
	if (GeneralSettings.PartType != RAIN)
	{
		LightningFlashIntensity = 0.0f;
		LightningFlashFramesRemaining = 0;
		return;
	}

	const double currentTime = GetCurrentTimeInSeconds();

	// Initialize lightning timing on first run
	if (NextLightningTime == 0.0)
	{
		// First lightning strike between 5-15 seconds, adjusted by frequency setting
		const double frequencyMultiplier = (101 - GeneralSettings.LightningFrequency) / 100.0; // Higher setting = more frequent
		NextLightningTime = currentTime + (5.0 + (rand() % 10)) * frequencyMultiplier;
	}

	// Check if it's time for lightning
	if (currentTime >= NextLightningTime)
	{
		// Trigger lightning flash with user-configurable intensity
		const float baseIntensity = 0.05f + (GeneralSettings.LightningIntensity / 100.0f) * 0.3f; // 0.05-0.35 range
		LightningFlashIntensity = baseIntensity + (rand() % 5) * 0.01f; // Add small random variation
		LightningFlashFramesRemaining = 3 + (rand() % 4); // 3-6 frames duration

		// Schedule next lightning with frequency setting (5-60 seconds range)
		const double frequencyMultiplier = (101 - GeneralSettings.LightningFrequency) / 100.0; // Higher setting = more frequent
		const double baseInterval = 5.0 + (rand() % 25); // 5-30 seconds base
		LastLightningTime = currentTime;
		NextLightningTime = currentTime + baseInterval * frequencyMultiplier;
	}

	// Fade out lightning flash
	if (LightningFlashFramesRemaining > 0)
	{
		LightningFlashFramesRemaining--;
		if (LightningFlashFramesRemaining == 0)
		{
			LightningFlashIntensity = 0.0f;
		}
		else
		{
			// Exponential fade out
			LightningFlashIntensity *= 0.7f;
		}
	}
}

void DisplayWindow::DrawLightningFlash() const
{
	if (LightningFlashIntensity <= 0.0f || GeneralSettings.PartType != RAIN)
	{
		return;
	}

	// Create a semi-transparent white brush for the flash
	const D2D1_COLOR_F flashColor = D2D1::ColorF(D2D1::ColorF::White, LightningFlashIntensity);
	ComPtr<ID2D1SolidColorBrush> flashBrush;
	
	if (SUCCEEDED(Dc->CreateSolidColorBrush(flashColor, &flashBrush)))
	{
		// Fill the entire screen with the flash
		const D2D1_RECT_F screenRect = D2D1::RectF(
			static_cast<float>(pDisplaySpecificData->SceneRect.left),
			static_cast<float>(pDisplaySpecificData->SceneRect.top),
			static_cast<float>(pDisplaySpecificData->SceneRect.right),
			static_cast<float>(pDisplaySpecificData->SceneRect.bottom)
		);
		
		Dc->FillRectangle(screenRect, flashBrush.Get());
	}
}

void DisplayWindow::UpdateZOrder(HWND hWnd)
{
    // Get the current foreground window
    HWND hwndForeground = GetForegroundWindow();
    
    // Skip if our window is the foreground window (shouldn't happen but just in case)
    if (hwndForeground == hWnd)
        return;
    
    // If there is an active foreground window, place our window just behind it
    if (hwndForeground != NULL)
    {
        // Position our window immediately below the foreground window
        SetWindowPos(hWnd, hwndForeground, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    else
    {
        // If there's no foreground window, make sure we're still above most windows
        // but below any that might become active
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        
        // Immediately undo the topmost status to allow other windows to go above us when activated
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}
