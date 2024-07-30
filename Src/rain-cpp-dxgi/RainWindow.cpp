#include "RainWindow.h"

#include <shellapi.h>
#include <commctrl.h>
#include "Resource.h"

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


HINSTANCE RainWindow::AppInstance = nullptr;
RainWindow* RainWindow::pThis;
OptionsDialog* RainWindow::pOptionsDlg;

void RainWindow::LoadOptionValues()
{
	MaxRainDrops = 10;
	RainDirection = 3;
	RainColor = 0x00AAAAAA;
}

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

	DWORD overlay_style = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST;
	// Using WS_EX_LAYERED loses the GPU-friendliness
	//DWORD normal_style = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST;

	const HWND window = CreateWindowEx(overlay_style,
	                                   wc.lpszClassName, L"let it rain",
	                                   WS_POPUP | WS_VISIBLE,
	                                   300, 200,
	                                   600, 400,
	                                   nullptr, nullptr, HINST_THISCOMPONENT, nullptr);

	ShowWindow(window, SW_MAXIMIZE);

	LoadOptionValues();

	pOptionsDlg = new OptionsDialog(AppInstance, this, MaxRainDrops, RainDirection, RainColor);
	pOptionsDlg->Create();

	InitDirect2D(window);

	RECT rc;
	GetClientRect(window, &rc);
	WindowWidth = rc.right - rc.left;
	WindowHeight = rc.bottom - rc.top;
	TaskBarHeight = GetTaskBarHeight();

	RainDrop::SetRainColor(Dc.Get(), RainColor);
	RainDrop::SetWindowBounds(WindowWidth, WindowHeight - TaskBarHeight);

	return 0;
}

LRESULT RainWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		{
			InitNotifyIcon(hWnd);
			break;
		}
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
			Paint();
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
	HMENU hMenu = CreatePopupMenu();
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
	                     &Direct3dDevice,
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
	constexpr D2D1_FACTORY_OPTIONS options = {D2D1_DEBUG_LEVEL_INFORMATION};
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

int RainWindow::GetTaskBarHeight()
{
	RECT rect;
	HWND taskBar = FindWindow(L"Shell_traywnd", nullptr);
	if (taskBar && GetWindowRect(taskBar, &rect))
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
	for (auto drop : RainDrops)
	{
		drop->Draw(Dc.Get());
		drop->MoveToNewPosition();
	}
}

void RainWindow::CheckAndGenerateRainDrops()
{
	// Move each raindrop to the next point
	for (auto drop : RainDrops)
	{
		drop->MoveToNewPosition();
	}

	// Remove all raindrops that have expired
	for (auto it = RainDrops.begin(); it != RainDrops.end();)
	{
		if ((*it)->ShouldBeErasedAndDeleted())
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
	for (auto drop : RainDrops)
	{
		if (!drop->DidDropLand())
		{
			countOfFallingDrops++;
		}
	}
	int noOfDropsToGenerate = MaxRainDrops - countOfFallingDrops;

	// Generate new raindrops
	for (int i = 0; i < noOfDropsToGenerate; ++i)
	{
		auto k = new RainDrop(RainDirection, RainDropType::MainDrop);
		RainDrops.push_back(k);
	}
}

void RainWindow::UpdateRainDropCount(int val)
{
	MaxRainDrops = val;
}

void RainWindow::UpdateRainDirection(int val)
{
	RainDirection = val;
}

void RainWindow::UpdateRainColor(COLORREF color)
{
	RainDrop::SetRainColor(Dc.Get(), color);
}
