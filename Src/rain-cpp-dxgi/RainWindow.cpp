#include "RainWindow.h"

#include <shellapi.h>
#include <commctrl.h>
#include <sstream>

#include "CPUUsageTracker.h"
#include "Resource.h"
#include "SettingsManager.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

struct ComException
{
	HRESULT result;

	ComException(const HRESULT value) :
		result(value)
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

#define DELAY_TIMER_1 1947
#define CPU_CHECK_TIMER 1948

HINSTANCE RainWindow::AppInstance = nullptr;
RainWindow* RainWindow::pThis;
OptionsDialog* RainWindow::pOptionsDlg;


HRESULT RainWindow::Initialize(HINSTANCE hInstance)
{
	pThis = this;
	AppInstance = hInstance;
	WNDCLASS wc = {};
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = HINST_THISCOMPONENT;
	wc.lpszClassName = L"window";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	RegisterClass(&wc);

	//WS_EX_TOOLWINDOW
	const DWORD exstyle = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST;
	//DWORD exstyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST;
	const DWORD style = WS_POPUP | WS_VISIBLE;

	const HWND window = CreateWindowEx(exstyle, wc.lpszClassName, L"let it rain", style,
	                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	                                   nullptr, nullptr, HINST_THISCOMPONENT, nullptr);

	ShowWindow(window, SW_MAXIMIZE);

	LoadOptionValues();

	pOptionsDlg = new OptionsDialog(AppInstance, this, settings.MaxRainDrops, settings.RainDirection, settings.RainColor);
	pOptionsDlg->Create();

	InitDirect2D(window);
	RainDrop::SetRainColor(Dc.Get(), settings.RainColor);
	HandleWindowBoundsChange(window, false);

	return 0;
}

void RainWindow::UpdateRainDropCount(int val)
{
	settings.MaxRainDrops = val;
}

void RainWindow::UpdateRainDirection(int val)
{
	settings.RainDirection = val;
}

void RainWindow::UpdateRainColor(COLORREF color)
{
	settings.RainColor = color;
	RainDrop::SetRainColor(Dc.Get(), color);
}

void RainWindow::LoadOptionValues()
{
	// Read settings (or create with default values)
	SettingsManager::GetInstance()->ReadSettings(settings);
}


LRESULT RainWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT_PTR timerId = 0;

	switch (message)
	{
	case WM_CREATE:
		{
			InitNotifyIcon(hWnd);
			SetTimer(hWnd, CPU_CHECK_TIMER, 500, nullptr);
			break;
		}
	case WM_DISPLAYCHANGE:
	case WM_DPICHANGED:
		{
			if (timerId != 0)
			{
				KillTimer(hWnd, timerId);
			}
			timerId = SetTimer(hWnd, DELAY_TIMER_1, 1000, nullptr);
			break;
		}
	case WM_TIMER:
		if (wParam == DELAY_TIMER_1)
		{
			pThis->HandleWindowBoundsChange(hWnd, true);
			KillTimer(hWnd, DELAY_TIMER_1);
			timerId = 0;
		}
		if (wParam == CPU_CHECK_TIMER)
		{
			HandleTaskBarHeightChange(hWnd);
			//HandleCPULoadChange(hWnd);			
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
	case WM_KEYDOWN: // Pressing any key will trigger a redraw 
		{
			InvalidateRect(hWnd, nullptr, false);
			break;
		}
	case WM_PAINT:
		{
			//Paint();
			break;
		}
	case WM_DESTROY:
		{
			SettingsManager::GetInstance()->WriteSettings(pThis->settings);
			PostQuitMessage(0);
			return 0;
		}
	default: ;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void RainWindow::RunMessageLoop()
{
	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (!cpuIsBusy)
			{
				Paint();
			}
			Sleep(10);
		}
	}
}


void RainWindow::InitNotifyIcon(HWND hWnd)
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

void RainWindow::ShowContextMenu(HWND hWnd)
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

void RainWindow::InitDirect2D(HWND hWnd)
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

	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
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

void RainWindow::HandleWindowBoundsChange(const HWND window, bool clearDrops)
{
	if (!RainDrops.empty() && clearDrops)
	{
		RainDrops.clear();
	}

	RECT rc;
	GetClientRect(window, &rc);
	WindowWidth = rc.right - rc.left;
	WindowHeight = rc.bottom - rc.top;
	TaskBarHeight = GetTaskBarHeight();
	const float scaleFactor = WindowHeight / 1080.0f;
	RainDrop::SetWindowBounds(WindowWidth, WindowHeight - TaskBarHeight, scaleFactor);


	//std::wostringstream output;
	//output << L"WindowWidth: " << WindowWidth << L"\n";
	//output << L"WindowHeight: " << WindowHeight << L"\n";
	//output << L"TaskBarHeight: " << TaskBarHeight << L"\n";
	//OutputDebugString(output.str().c_str());
}

void RainWindow::HandleTaskBarHeightChange(HWND hWnd)
{
	static int previousTaskBarHeight = -1;
	const int taskBarHeight = GetTaskBarHeight();
	if (previousTaskBarHeight == -1)
	{
		previousTaskBarHeight = taskBarHeight;
	}
	else if (previousTaskBarHeight != taskBarHeight)
	{
		previousTaskBarHeight = taskBarHeight;
		pThis->HandleWindowBoundsChange(hWnd, false);
	}
}

void RainWindow::HandleCPULoadChange(HWND hWnd)
{
	static int cpuFreeCycles = 1;
	static bool windowHidden = false;
	if (CPUUsageTracker::getInstance().GetCPUUsage() > 80)
	{
		if (!windowHidden)
		{
			cpuFreeCycles = 0;
			pThis->cpuIsBusy = true;
			ShowWindow(hWnd, SW_HIDE);
			windowHidden = true;
			if (!pThis->RainDrops.empty())
			{
				pThis->RainDrops.clear();
			}
		}
	}
	else
	{
		cpuFreeCycles++;
		if (cpuFreeCycles > 3)
		{
			pThis->cpuIsBusy = false;
			ShowWindow(hWnd, SW_SHOW);
			windowHidden = false;
		}
	}
}

bool RainWindow::IsTaskBarVisible()
{
	const HWND hTaskbarWnd = FindWindow(L"Shell_traywnd", nullptr);
	const HMONITOR hMonitor = MonitorFromWindow(hTaskbarWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info = {sizeof(MONITORINFO)};
	if (GetMonitorInfo(hMonitor, &info))
	{
		RECT rect;
		GetWindowRect(hTaskbarWnd, &rect);
		if ((rect.top >= info.rcMonitor.bottom - 4) ||
			(rect.right <= 2) ||
			(rect.bottom <= 4) ||
			(rect.left >= info.rcMonitor.right - 2))
			return false;

		return true;
	}
	return false;
}

int RainWindow::GetTaskBarHeight()
{
	RECT rect;
	const HWND taskBar = FindWindow(L"Shell_traywnd", nullptr);
	if (taskBar && GetWindowRect(taskBar, &rect) && IsTaskBarVisible())
	{
		return rect.bottom - rect.top;
	}
	return 0;
}

void RainWindow::Paint()
{
	Dc->BeginDraw();
	Dc->Clear();
	DrawRainDrops();
	HR(Dc->EndDraw());
	// Make the swap chain available to the composition engine
	HR(SwapChain->Present(1, 0));
}


void RainWindow::DrawRainDrops()
{
	CheckAndGenerateRainDrops();
	for (const auto drop : RainDrops)
	{
		drop->Draw(Dc.Get());
		drop->MoveToNewPosition();
	}
}

void RainWindow::CheckAndGenerateRainDrops()
{
	// Move each raindrop to the next point
	for (const auto drop : RainDrops)
	{
		drop->MoveToNewPosition();
	}

	// Remove all raindrops that have expired
	for (auto it = RainDrops.begin(); it != RainDrops.end();)
	{
		if ((*it)->IsReadyForErase())
		{
			delete*it;
			it = RainDrops.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Calculate the number of raindrops to generate
	int countOfFallingDrops = 0;
	for (const auto drop : RainDrops)
	{
		if (!drop->DidTouchGround())
		{
			countOfFallingDrops++;
		}
	}
	const int noOfDropsToGenerate = settings.MaxRainDrops - countOfFallingDrops;

	// Generate new raindrops
	for (int i = 0; i < noOfDropsToGenerate; ++i)
	{
		auto k = new RainDrop(settings.RainDirection, RainDropType::MainDrop);
		RainDrops.push_back(k);
	}
}
