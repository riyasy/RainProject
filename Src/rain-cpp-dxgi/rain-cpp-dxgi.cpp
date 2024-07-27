#include "rain-cpp-dxgi.h"

//
// Provides the entry point to the application.
//
int WINAPI WinMain(
	HINSTANCE /* hInstance */,
	HINSTANCE /* hPrevInstance */,
	LPSTR /* lpCmdLine */,
	int /* nCmdShow */
)
{
	// Ignore the return value because we want to continue running even in the
	// unlikely event that HeapSetInformation fails.
	HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			let_it_rain app;

			if (SUCCEEDED(app.initialize()))
			{
				app.run_message_loop();
			}
		}
		CoUninitialize();
	}

	return 0;
}


HRESULT let_it_rain::initialize()
{
	WNDCLASS wc = {};
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = HINST_THISCOMPONENT;
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
					InvalidateRect(window, nullptr, false);
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

	init_direct_2d(window);

	RECT rc;
	GetClientRect(window, &rc);
	window_width_ = static_cast<float>(rc.right - rc.left);
	window_height_ = static_cast<float>(rc.bottom - rc.top);
	task_bar_height_ = get_task_bar_height();

	return 0;
}

void let_it_rain::run_message_loop()
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
			paint();
			Sleep(10);
		}
	}
}


void let_it_rain::init_direct_2d(HWND window)
{
	HR(D3D11CreateDevice(nullptr, // Adapter
	                     D3D_DRIVER_TYPE_HARDWARE,
	                     nullptr, // Module
	                     D3D11_CREATE_DEVICE_BGRA_SUPPORT,
	                     nullptr, 0, // Highest available feature level
	                     D3D11_SDK_VERSION,
	                     &direct3d_device_,
	                     nullptr, // Actual feature level
	                     nullptr)); // Device context

	HR(direct3d_device_.As(&dxgi_device_));

	HR(CreateDXGIFactory2(
		0,
		__uuidof(dx_factory_),
		reinterpret_cast<void**>(dx_factory_.GetAddressOf())));

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

	HR(dx_factory_->CreateSwapChainForComposition(dxgi_device_.Get(),
	                                            &description,
	                                            nullptr,
	                                            swap_chain_.GetAddressOf()));

	// Create a single-threaded Direct2D factory with debugging information
	constexpr D2D1_FACTORY_OPTIONS options = {D2D1_DEBUG_LEVEL_INFORMATION};
	HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
	                     options,
	                     d2_factory_.GetAddressOf()));
	// Create the Direct2D device that links back to the Direct3D device
	HR(d2_factory_->CreateDevice(dxgi_device_.Get(),
	                           d_2device_.GetAddressOf()));
	// Create the Direct2D device context that is the actual render target
	// and exposes drawing commands
	HR(d_2device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
	                                 dc_.GetAddressOf()));
	// Retrieve the swap chain's back buffer
	HR(swap_chain_->GetBuffer(
		0, // index
		__uuidof(surface_),
		reinterpret_cast<void**>(surface_.GetAddressOf())));
	// Create a Direct2D bitmap that points to the swap chain surface
	D2D1_BITMAP_PROPERTIES1 properties = {};
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET |
		D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	HR(dc_->CreateBitmapFromDxgiSurface(surface_.Get(),
	                                   properties,
	                                   bitmap_.GetAddressOf()));
	// Point the device context to the bitmap for rendering
	dc_->SetTarget(bitmap_.Get());

	HR(DCompositionCreateDevice(
		dxgi_device_.Get(),
		__uuidof(dcomp_device_),
		reinterpret_cast<void**>(dcomp_device_.GetAddressOf())));


	HR(dcomp_device_->CreateTargetForHwnd(window,
	                                    true, // Top most
	                                    target_.GetAddressOf()));

	HR(dcomp_device_->CreateVisual(visual_.GetAddressOf()));
	HR(visual_->SetContent(swap_chain_.Get()));
	HR(target_->SetRoot(visual_.Get()));
	HR(dcomp_device_->Commit());

	//D2D1_COLOR_F const brushColor = D2D1::ColorF(0.18f,  // red
	//	0.55f,  // green
	//	0.34f,  // blue
	//	0.75f); // alpha

	//HR(dc->CreateSolidColorBrush(brushColor,
	//	brush.GetAddressOf()));

	HR(dc_->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::LightGray),
		brush_.GetAddressOf()
	));
}

int let_it_rain::get_task_bar_height()
{
	RECT rect;
	HWND taskBar = FindWindow(L"Shell_traywnd", nullptr);
	if (taskBar && GetWindowRect(taskBar, &rect))
	{
		return rect.bottom - rect.top;
	}
	return 0;
}

void let_it_rain::paint()
{
	dc_->BeginDraw();
	dc_->Clear();
	draw_rain_drops();
	HR(dc_->EndDraw());
	// Make the swap chain available to the composition engine
	HR(swap_chain_->Present(1, 0));
}

void let_it_rain::draw_rain_drops()
{
	check_and_generate_rain_drops();
	for (auto drop : rain_drops_)
	{
		drop->draw(dc_.Get(), brush_.Get());
		drop->move_to_new_position();
	}
}

void let_it_rain::check_and_generate_rain_drops()
{
	// Move each raindrop to the next point
	for (auto drop : rain_drops_)
	{
		drop->move_to_new_position();
	}

	// Remove all raindrops that have expired
	for (auto it = rain_drops_.begin(); it != rain_drops_.end();)
	{
		if ((*it)->should_be_erased_and_deleted())
		{
			delete*it;
			it = rain_drops_.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Calculate the number of raindrops to generate
	int countOfFallingDrops = 0;
	for (auto drop : rain_drops_)
	{
		if (!drop->did_drop_land())
		{
			countOfFallingDrops++;
		}
	}
	int noOfDropsToGenerate = 10 - countOfFallingDrops;

	// Generate new raindrops
	for (int i = 0; i < noOfDropsToGenerate; ++i)
	{
		auto k = new rain_drop(window_width_, window_height_ - task_bar_height_, rain_drop_type::main_drop);
		rain_drops_.push_back(k);
	}
}
