#pragma once

#include <wrl.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <dcomp.h>
#include <vector>

#include "framework.h"
#include "RainDrop.h"
#include "CallBackWindow.h"
#include "OptionDialog.h"
#include "SettingsManager.h"


// https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine

using namespace Microsoft::WRL;


// Use a guid to uniquely identify our icon
class __declspec(uuid("9D0B8B92-4E1C-488e-A1E1-2331AFCE2CB5")) RainIcon;

class RainWindow final : CallBackWindow
{
public:
	HRESULT Initialize(HINSTANCE hInstance);
	void RunMessageLoop();

	void UpdateRainDropCount(int val) override;
	void UpdateRainDirection(int val) override;
	void UpdateRainColor(COLORREF color) override;

private:
	ComPtr<ID3D11Device> Direct3dDevice;
	ComPtr<IDXGIDevice> DxgiDevice;
	ComPtr<IDXGIFactory2> DxFactory;
	ComPtr<IDXGISwapChain1> SwapChain;
	ComPtr<ID2D1Factory2> D2Factory;
	ComPtr<ID2D1Device1> D2Device;
	ComPtr<ID2D1DeviceContext> Dc;
	ComPtr<IDXGISurface2> Surface;
	ComPtr<ID2D1Bitmap1> Bitmap;
	ComPtr<IDCompositionDevice> DcompDevice;
	ComPtr<IDCompositionTarget> Target;
	ComPtr<IDCompositionVisual> Visual;
	bool cpuIsBusy = false;

	static HINSTANCE AppInstance;
	static RainWindow* pThis;
	static OptionsDialog* pOptionsDlg;

	std::vector<RainDrop*> RainDrops;

	int WindowWidth = 100;
	int WindowHeight = 100;
	int TaskBarHeight = 0;

	Setting settings;

	//int MaxRainDrops = 10;
	//int RainDirection = 3;
	//COLORREF RainColor = 0x00AAAAAA;

	static LRESULT CALLBACK WndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);

	void LoadOptionValues();

	void HandleWindowBoundsChange(HWND window, bool clearDrops);
	static void HandleTaskBarHeightChange(HWND hWnd);
	static void HandleCPULoadChange(HWND hWnd);

	static void InitNotifyIcon(HWND hWnd);
	void InitDirect2D(HWND hWnd);

	static void ShowContextMenu(HWND hWnd);

	static int GetTaskBarHeight();
	static bool IsTaskBarVisible();

	void DrawRainDrops();
	void CheckAndGenerateRainDrops();
	void Paint();
};
