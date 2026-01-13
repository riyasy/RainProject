#include "DisplayWindow.h"

#include <chrono>
#include <shellapi.h>
#include <commctrl.h>
#include <sstream>
#include <memory>

#include "CPUUsageTracker.h"
#include "Global.h"
#include "MathUtil.h"
#include "Resource.h"
#include "SettingsManager.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#define NOTIFICATION_TRAY_ICON_UID 786;

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
	INTERVAL_TIMER = 1948
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

	constexpr DWORD exstyle = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
	//DWORD exstyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST;
	constexpr DWORD style = WS_POPUP | WS_VISIBLE;

	const HWND window = CreateWindowEx(exstyle, wc.lpszClassName, L"let it rain", style,
	                                   monitorData.MonitorRect.left,
	                                   monitorData.MonitorRect.top,
	                                   monitorData.MonitorRect.right - monitorData.MonitorRect.left,
	                                   monitorData.MonitorRect.bottom - monitorData.MonitorRect.top,
	                                   nullptr, nullptr, HINST_THISCOMPONENT, this);

	ShowWindow(window, SW_SHOW);

	if (!GeneralSettings.loaded)
	{
		SettingsManager::GetInstance()->ReadSettings(GeneralSettings);
	}

	OptionsDialog::SubscribeToChange(this);
	if (MonitorDat.IsPrimaryDisplay)
	{
		pOptionsDlg = new OptionsDialog(AppInstance, GeneralSettings.MaxParticles, GeneralSettings.WindSpeed,
		                                GeneralSettings.ParticleColor, GeneralSettings.PartType,
		                                GeneralSettings.StartWithWindows);
		pOptionsDlg->Create();
	}

	InitDirect2D(window);
	pDisplaySpecificData = std::make_unique<DisplayData>(Dc.Get());
	pDisplaySpecificData->SetRainColor(GeneralSettings.ParticleColor);
	HandleWindowBoundsChange(window, false);

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
			break;
		}
	case WM_DESTROY:
		{
			const DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			if (pThis->MonitorDat.IsPrimaryDisplay)
			{
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
			break;
		}
	case WM_TIMER:
		if (wParam == DELAY_TIMER)
		{
			DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			pThis->HandleWindowBoundsChange(hWnd, true);
			KillTimer(hWnd, DELAY_TIMER);
			timerId = 0;
		}
		if (wParam == INTERVAL_TIMER)
		{
			DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			pThis->HandleTaskBarChange();
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
	constexpr double dt = 0.01;

	if (CurrentTime < 0)
	{
		CurrentTime = GetCurrentTimeInSeconds();
	}
	const double newTime = GetCurrentTimeInSeconds();
	const double frameTime = newTime - CurrentTime;
	CurrentTime = newTime;

	Accumulator += frameTime;

	if (Accumulator > 1) 
	{
		// If there are large gaps in animation due to screen stuck
		// If Accumulator value is not decreased, large number of
		// Updates will be called in while loop
		Accumulator = dt;
	}

	while (Accumulator >= dt)
	{
		if (GeneralSettings.PartType == RAIN)
		{
			UpdateRainDrops();
		}
		else if (GeneralSettings.PartType == SNOW)
		{
			UpdateSnowFlakes();
		}
		Accumulator -= dt;
	}
	if (GeneralSettings.PartType == RAIN)
	{
		DrawRainDrops();
	}
	else if (GeneralSettings.PartType == SNOW)
	{
		DrawSnowFlakes();
	}	
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

	HR(DcompDevice->CreateTargetForHwnd(hWnd,
	                                    true, // Top most
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
	FindSceneRect(sceneRect, scaleFactor);
	if (sceneRect != pDisplaySpecificData->SceneRect)
	{
		if (!RainDrops.empty() && clearDrops)
		{
			// clear existing value-based drops
			RainDrops.clear();
		}
		pDisplaySpecificData->SetSceneBounds(sceneRect, scaleFactor);

		// Reserve memory to avoid reallocations and fragmentation
		RainDrops.reserve(GeneralSettings.MaxParticles * 3);

		//std::wostringstream  oss;
		//oss << "Monitor Name: " << MonitorDat.Name.c_str() << ", "
		//	<< "SceneRect Bounds: "
		//	<< "Left: " << sceneRect.left << ", "
		//	<< "Top: " << sceneRect.top << ", "
		//	<< "Right: " << sceneRect.right << ", "
		//	<< "Bottom: " << sceneRect.bottom << "\n";;
		//std::wstring logMessage = oss.str();
		//OutputDebugStringW(logMessage.c_str());
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

		//std::wostringstream  oss;
		//oss << "Monitor Name: " << MonitorDat.Name.c_str() << ", "
		//	<< "SceneRect Bounds: "
		//	<< "Left: " << sceneRect.left << ", "
		//	<< "Top: " << sceneRect.top << ", "
		//	<< "Right: " << sceneRect.right << ", "
		//	<< "Bottom: " << sceneRect.bottom << "\n";
		//std::wstring logMessage = oss.str();
		//OutputDebugStringW(logMessage.c_str());
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

	for (const auto& drop : RainDrops)
	{
		drop.Draw(Dc.Get());
	}

	HR(Dc->EndDraw());
	// Make the swap chain available to the composition engine
	HR(SwapChain->Present(1, 0));
}

void DisplayWindow::DrawSnowFlakes() const
{
	Dc->BeginDraw();
	Dc->Clear();

	for (const auto & flake : SnowFlakes)
	{
		flake.Draw(Dc.Get());
	}

	if (!SnowFlakes.empty())
	{
		SnowFlake::DrawSettledSnow(Dc.Get(), pDisplaySpecificData.get());
	}

	HR(Dc->EndDraw());
	// Make the swap chain available to the composition engine
	HR(SwapChain->Present(1, 0));
}

void DisplayWindow::UpdateRainDrops()
{
	// Move each raindrop to the next point
	for (auto & drop : RainDrops)
	{
		drop.UpdatePosition(0.01f);
	}

	// Remove all raindrops that have expired using swap-and-pop to avoid shifting
	for (size_t i = 0; i < RainDrops.size(); )
	{
		if (RainDrops[i].IsReadyForErase())
		{
			// Move last element into position i and pop
			if (i + 1 != RainDrops.size())
			{
				RainDrops[i] = std::move(RainDrops.back());
			}
			RainDrops.pop_back();
			// do not increment i, re-evaluate new element at i
		}
		else
		{
			++i;
		}
	}

	// Calculate the number of raindrops to generate
	int countOfFallingDrops = 0;
	for (const auto & drop : RainDrops)
	{
		if (!drop.DidTouchGround())
		{
			countOfFallingDrops++;
		}
	}

	const int noOfDropsToGenerate = GeneralSettings.MaxParticles * 3 - countOfFallingDrops;

	// Generate new raindrops (emplace so constructor runs in-place)
	if (noOfDropsToGenerate > 0)
	{
		for (int i = 0; i < noOfDropsToGenerate; ++i)
		{
			RainDrops.emplace_back(GeneralSettings.WindSpeed, pDisplaySpecificData.get());
		}
	}
	else if (noOfDropsToGenerate < 0)
	{
		// If we have too many drops, remove some dead/falling ones. We'll pop from the back.
		int excess = -noOfDropsToGenerate;
		while (excess > 0 && !RainDrops.empty())
		{
			RainDrops.pop_back();
			--excess;
		}
	}
}

void DisplayWindow::UpdateSnowFlakes()
{
	const int noOfFlakesToGenerate = GeneralSettings.MaxParticles * 20 - static_cast<int>(SnowFlakes.size());

	if (noOfFlakesToGenerate > 0)
	{
		for (int i = 0; i < noOfFlakesToGenerate; i++)
		{
			SnowFlakes.emplace_back(pDisplaySpecificData.get());
		}
	}

	if (noOfFlakesToGenerate < 0)
	{
		int excess = -noOfFlakesToGenerate;
		// Remove using swap-and-pop from vector's back
		while (excess > 0 && !SnowFlakes.empty())
		{
			SnowFlakes.pop_back();
			--excess;
		}
	}


	// Move each snowflake to the next point
	for (auto & flake : SnowFlakes)
	{
		flake.UpdatePosition(0.01f, CurrentTime);
	}
	SnowFlake::SettleSnow(pDisplaySpecificData.get());
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
	// unique_ptr will clean up automatically
};
