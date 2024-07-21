#pragma once

#include <windows.h>
#pragma comment(lib, "user32.lib")
#include <wrl.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <dcomp.h>
#include <vector>
#include "RainDrop.h"
#include <chrono>


#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")

// https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine

using namespace Microsoft::WRL;

struct ComException
{
	HRESULT result;
	ComException(HRESULT const value) :
		result(value)
	{}
};
void HR(HRESULT const result)
{
	if (S_OK != result)
	{
		throw ComException(result);
	}
}

ComPtr<ID3D11Device> direct3dDevice;
ComPtr<IDXGIDevice> dxgiDevice;
ComPtr<IDXGIFactory2> dxFactory;
ComPtr<IDXGISwapChain1> swapChain;
ComPtr<ID2D1Factory2> d2Factory;
ComPtr<ID2D1Device1> d2Device;
ComPtr<ID2D1DeviceContext> dc;
ComPtr<IDXGISurface2> surface;
ComPtr<ID2D1Bitmap1> bitmap;
ComPtr<IDCompositionDevice> dcompDevice;
ComPtr<IDCompositionTarget> target;
ComPtr<IDCompositionVisual> visual;
ComPtr<ID2D1SolidColorBrush> brush;

// Global Variables
std::vector<rain_drop*> rain_drops;
// Constants
float window_width = 100;
float window_height = 100;
int task_bar_height = 0;

void InitDirect2D(HWND window)
{
	HR(D3D11CreateDevice(nullptr,    // Adapter
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,    // Module
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr, 0, // Highest available feature level
		D3D11_SDK_VERSION,
		&direct3dDevice,
		nullptr,    // Actual feature level
		nullptr));  // Device context

	HR(direct3dDevice.As(&dxgiDevice));

	HR(CreateDXGIFactory2(
		DXGI_CREATE_FACTORY_DEBUG,
		__uuidof(dxFactory),
		reinterpret_cast<void**>(dxFactory.GetAddressOf())));

	DXGI_SWAP_CHAIN_DESC1 description = {};
	description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	description.BufferCount = 2;
	description.SampleDesc.Count = 1;
	description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

	RECT rect = {};
	GetClientRect(window, &rect);
	description.Width = rect.right - rect.left;
	description.Height = rect.bottom - rect.top;

	HR(dxFactory->CreateSwapChainForComposition(dxgiDevice.Get(),
		&description,
		nullptr, // Don’t restrict
		swapChain.GetAddressOf()));

	// Create a single-threaded Direct2D factory with debugging information
	D2D1_FACTORY_OPTIONS const options = { D2D1_DEBUG_LEVEL_INFORMATION };
	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		options,
		d2Factory.GetAddressOf()));
	// Create the Direct2D device that links back to the Direct3D device
	HR(d2Factory->CreateDevice(dxgiDevice.Get(),
		d2Device.GetAddressOf()));
	// Create the Direct2D device context that is the actual render target
	// and exposes drawing commands
	HR(d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
		dc.GetAddressOf()));
	// Retrieve the swap chain's back buffer
	HR(swapChain->GetBuffer(
		0, // index
		__uuidof(surface),
		reinterpret_cast<void**>(surface.GetAddressOf())));
	// Create a Direct2D bitmap that points to the swap chain surface
	D2D1_BITMAP_PROPERTIES1 properties = {};
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET |
		D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	HR(dc->CreateBitmapFromDxgiSurface(surface.Get(),
		properties,
		bitmap.GetAddressOf()));
	// Point the device context to the bitmap for rendering
	dc->SetTarget(bitmap.Get());

	HR(DCompositionCreateDevice(
		dxgiDevice.Get(),
		__uuidof(dcompDevice),
		reinterpret_cast<void**>(dcompDevice.GetAddressOf())));


	HR(dcompDevice->CreateTargetForHwnd(window,
		true, // Top most
		target.GetAddressOf()));

	HR(dcompDevice->CreateVisual(visual.GetAddressOf()));
	HR(visual->SetContent(swapChain.Get()));
	HR(target->SetRoot(visual.Get()));
	HR(dcompDevice->Commit());

	//D2D1_COLOR_F const brushColor = D2D1::ColorF(0.18f,  // red
	//	0.55f,  // green
	//	0.34f,  // blue
	//	0.75f); // alpha

	//HR(dc->CreateSolidColorBrush(brushColor,
	//	brush.GetAddressOf()));

	dc->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::LightGray),
		brush.GetAddressOf()
	);

}

int getTaskBarHeight()
{
	RECT rect;
	HWND taskBar = FindWindow(L"Shell_traywnd", NULL);
	if (taskBar && GetWindowRect(taskBar, &rect)) {
		return rect.bottom - rect.top;
	}
	return 0;
}

void update_rain_drops();
void draw_rain_drops();
void check_and_generate_rain_drops();
void paint();

void paint()
{
	dc->BeginDraw();
	dc->Clear();
	draw_rain_drops();
	HR(dc->EndDraw());
	// Make the swap chain available to the composition engine
	HR(swapChain->Present(1, 0));
}

int __stdcall wWinMain(HINSTANCE module, HINSTANCE, PWSTR, int)
{
	WNDCLASS wc = {};
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = module;
	wc.lpszClassName = L"window";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc =
		[](HWND window, UINT message, const WPARAM wparam,
		   const LPARAM lparam) -> LRESULT
		{
			switch (message)
			{
			case WM_NCHITTEST:
			{
				LRESULT hit = DefWindowProc(window, message, wparam, lparam);
				if (hit == HTCLIENT) hit = HTCAPTION;
				return hit;
			}
			case WM_KEYDOWN: // Pressing any key will trigger a redraw 
			{
				::InvalidateRect(window, nullptr, false);
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
			}
			return DefWindowProc(window, message, wparam, lparam);
		};
	RegisterClass(&wc);

	DWORD overlay_style = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST; // Using WS_EX_LAYERED loses the GPU-friendliness
	DWORD normal_style = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST;

	HWND const window = CreateWindowEx(overlay_style,
		wc.lpszClassName, L"Sample",
		WS_POPUP | WS_VISIBLE,
		300, 200,
		600, 400,
		nullptr, nullptr, module, nullptr);

	ShowWindow(window, SW_MAXIMIZE);

	InitDirect2D(window);

	RECT rc;
	GetClientRect(window, &rc);
	window_width = static_cast<float>(rc.right - rc.left);
	window_height = static_cast<float>(rc.bottom - rc.top);
	task_bar_height = getTaskBarHeight();

	MSG msg = {};

	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			paint();
			Sleep(10);
		}
	}
	return 0;
}

void draw_rain_drops() {
	check_and_generate_rain_drops();
	for (auto drop : rain_drops) {
		drop->draw(dc.Get(), brush.Get());
	}
	for (auto drop : rain_drops) {
		drop->move_to_new_position();
	}
}

void check_and_generate_rain_drops() {
	// Move each raindrop to the next point
	for (auto drop : rain_drops) {
		drop->move_to_new_position();
	}

	// Remove all raindrops that have expired

	for (std::vector<rain_drop*>::iterator it = rain_drops.begin(); it != rain_drops.end(); ) {
		if ((*it)->should_be_erased_and_deleted()) {
			delete* it;
			it = rain_drops.erase(it);
		}
		else {
			++it;
		}
	}

	// Calculate the number of raindrops to generate
	int countOfFallingDrops = 0;
	for (auto drop : rain_drops) {
		if (!drop->did_drop_land())
		{
			countOfFallingDrops++;
		}
	}
	int noOfDropsToGenerate = 10 - countOfFallingDrops;

	// Generate new raindrops
	for (int i = 0; i < noOfDropsToGenerate; ++i) {
		rain_drop* k = new rain_drop(window_width, window_height - task_bar_height, rain_drop_type::main_drop);
		rain_drops.push_back(k);
	}
}