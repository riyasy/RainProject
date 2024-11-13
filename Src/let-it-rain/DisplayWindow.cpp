#include "DisplayWindow.h"

#include <chrono>
#include <shellapi.h>
#include <commctrl.h>
#include <sstream>

#include "CPUUsageTracker.h"
#include "MathUtil.h"
#include "Resource.h"
#include "SettingsManager.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

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

enum
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
	                                   monitorData.DisplayRect.left,
	                                   monitorData.DisplayRect.top,
	                                   monitorData.DisplayRect.right - monitorData.DisplayRect.left,
	                                   monitorData.DisplayRect.bottom - monitorData.DisplayRect.top,
	                                   nullptr, nullptr, HINST_THISCOMPONENT, this);

	ShowWindow(window, SW_SHOW);

	if (!GeneralSettings.loaded)
	{
		SettingsManager::GetInstance()->ReadSettings(GeneralSettings);
	}

	OptionsDialog::SubscribeToChange(this);
	if (MonitorDat.IsDefaultDisplay)
	{
		pOptionsDlg = new OptionsDialog(AppInstance, GeneralSettings.MaxRainDrops, GeneralSettings.RainDirection,
		                                GeneralSettings.RainColor);
		pOptionsDlg->Create();
	}

	InitDirect2D(window);
	pDisplaySpecificData = new DisplayData(Dc.Get());
	pDisplaySpecificData->SetRainColor(GeneralSettings.RainColor);
	HandleWindowBoundsChange(window, false);

	return 0;
}

void DisplayWindow::UpdateRainDropCount(const int val)
{
	GeneralSettings.MaxRainDrops = val;
}

void DisplayWindow::UpdateRainDirection(const int val)
{
	GeneralSettings.RainDirection = val;
}

void DisplayWindow::UpdateRainColor(const COLORREF color)
{
	GeneralSettings.RainColor = color;
	pDisplaySpecificData->SetRainColor(color);
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
			DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			if (pThis->MonitorDat.IsDefaultDisplay)
			{
				InitNotifyIcon(hWnd);
			}
			SetTimer(hWnd, INTERVAL_TIMER, 500, nullptr);
			break;
		}
	case WM_DESTROY:
		{
			DisplayWindow* pThis = GetInstanceFromHwnd(hWnd);
			if (pThis->MonitorDat.IsDefaultDisplay)
			{
				SettingsManager::GetInstance()->WriteSettings(GeneralSettings);
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

	while (Accumulator >= dt)
	{
		//UpdateRainDrops();
		UpdateSnowFlakes();
		Accumulator -= dt;
	}
	//DrawRainDrops();
	DrawSnowFlakes();
}

void DisplayWindow::InitNotifyIcon(const HWND hWnd)
{
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.hWnd = hWnd;
	// add the icon, setting the icon, tooltip, and callback message.
	// the icon will be identified with the GUID
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = __uuidof(RainIcon);
	nid.uCallbackMessage = WM_TRAYICON;
	LoadIconMetric(AppInstance, MAKEINTRESOURCE(IDI_SMALL), LIM_SMALL, &nid.hIcon);

	//LoadString(appInstance, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	Shell_NotifyIcon(NIM_ADD, &nid);

	// NOTIFYICON_VERSION_4 is prefered
	nid.uVersion = NOTIFYICON_VERSION_4;
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
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
	if (!RainDrops.empty() && clearDrops)
	{
		RainDrops.clear();
	}
	RECT rainableRect;
	float scaleFactor = 1.0f;
	FindRainableRect(rainableRect, scaleFactor);
	pDisplaySpecificData->SetWindowBounds(rainableRect, scaleFactor);
}

void DisplayWindow::HandleTaskBarChange()
{
	RECT rainableRect;
	float scaleFactor = 1.0f;
	FindRainableRect(rainableRect, scaleFactor);
	pDisplaySpecificData->SetWindowBounds(rainableRect, scaleFactor);
}

void DisplayWindow::FindRainableRect(RECT& rainableRect, float& scaleFactor)
{
	HWND hTaskbarWnd;
	if (MonitorDat.IsDefaultDisplay)
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
		rainableRect = desktopRect;
	}
	else
	{
		rainableRect = MathUtil::SubtractRect(desktopRect, taskBarRect);
	}

	rainableRect = MathUtil::NormalizeRect(rainableRect, top, left);

	const int monitorHeight = info.rcMonitor.bottom - info.rcMonitor.top;
	scaleFactor = static_cast<float>(monitorHeight) / 1080.0f;
}

void DisplayWindow::DrawRainDrops() const
{
	Dc->BeginDraw();
	Dc->Clear();

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

	for (const auto pFlake : SnowFlakes)
	{
		pFlake->Draw(Dc.Get());
	}

	if (!SnowFlakes.empty())
	{
		SnowFlake::DrawSettledSnow(Dc.Get(), pDisplaySpecificData);
	}

	HR(Dc->EndDraw());
	// Make the swap chain available to the composition engine
	HR(SwapChain->Present(1, 0));
}

void DisplayWindow::UpdateRainDrops()
{
	// Move each raindrop to the next point
	for (RainDrop* const pDrop : RainDrops)
	{
		pDrop->UpdatePosition(0.01f);
	}

	// Remove all raindrops that have expired
	for (auto pDropIterator = RainDrops.begin(); pDropIterator != RainDrops.end();)
	{
		if ((*pDropIterator)->IsReadyForErase())
		{
			delete*pDropIterator;
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

	const int noOfDropsToGenerate = GeneralSettings.MaxRainDrops - countOfFallingDrops;

	// Generate new raindrops
	for (int i = 0; i < noOfDropsToGenerate; ++i)
	{
		RainDrop* pDrop = new RainDrop(GeneralSettings.RainDirection, pDisplaySpecificData);
		RainDrops.push_back(pDrop);
	}
}

void DisplayWindow::UpdateSnowFlakes()
{
	if (SnowFlakes.empty())
	{
		for (int i = 0; i < 10000; i++)
		{
			SnowFlake* pFlake = new SnowFlake(pDisplaySpecificData);
			SnowFlakes.push_back(pFlake);
		}
	}
	// Move each raindrop to the next point
	for (SnowFlake* const pFlake : SnowFlakes)
	{
		pFlake->UpdatePosition(0.01f);
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

DisplayWindow::~DisplayWindow() = default;
