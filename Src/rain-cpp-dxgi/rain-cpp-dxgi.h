#pragma once


#include "framework.h"
#include "RainDrop.h"

#include <wrl.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <dcomp.h>

#include <vector>
#include <chrono>


// https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine

using namespace Microsoft::WRL;

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

class let_it_rain
{
public:
	HRESULT initialize();
	void run_message_loop();

private:
	ComPtr<ID3D11Device> direct3d_device_;
	ComPtr<IDXGIDevice> dxgi_device_;
	ComPtr<IDXGIFactory2> dx_factory_;
	ComPtr<IDXGISwapChain1> swap_chain_;
	ComPtr<ID2D1Factory2> d2_factory_;
	ComPtr<ID2D1Device1> d_2device_;
	ComPtr<ID2D1DeviceContext> dc_;
	ComPtr<IDXGISurface2> surface_;
	ComPtr<ID2D1Bitmap1> bitmap_;
	ComPtr<IDCompositionDevice> dcomp_device_;
	ComPtr<IDCompositionTarget> target_;
	ComPtr<IDCompositionVisual> visual_;
	ComPtr<ID2D1SolidColorBrush> brush_;

	std::vector<rain_drop*> rain_drops_;
	float window_width_ = 100;
	float window_height_ = 100;
	int task_bar_height_ = 0;

	void init_direct_2d(HWND window);
	static int get_task_bar_height();
	void draw_rain_drops();
	void check_and_generate_rain_drops();
	void paint();
};
