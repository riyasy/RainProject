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
#include "SnowFlake.h"

// https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine

using namespace Microsoft::WRL;

// Use a guid to uniquely identify our icon
class __declspec(uuid("355F4E1D-8039-4078-BABD-8668FD2D1F7B")) RainIcon;

struct MonitorData
{
	RECT MonitorRect; // The display's rectangle dimensions
	std::wstring Name; // The display name, if available
	bool IsPrimaryDisplay; // True if the display is the primary one

	MonitorData() : MonitorRect{0, 0, 0, 0}, IsPrimaryDisplay(false)
	{
	}
};

class DisplayWindow final : CallBackWindow
{
public:
	HRESULT Initialize(HINSTANCE hInstance, const MonitorData& monitorData);
	void Animate();

	// CallBackWindow Overrides
	void UpdateParticleCount(int val) override;
	void UpdateWindDirection(int val) override;
	void UpdateParticleColor(COLORREF color) override;
	void UpdateParticleType(ParticleType partType) override;
	void UpdateLightningFrequency(int val) override;
	void UpdateLightningIntensity(int val) override;

	// Snow wind settings callbacks
	void UpdateEnableSnowWind(bool enabled) override;
	void UpdateSnowWindIntensity(int val) override;
	void UpdateSnowWindVariability(int val) override;

	~DisplayWindow() override;

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

	static HINSTANCE AppInstance;
	static OptionsDialog* pOptionsDlg;

	std::vector<RainDrop*> RainDrops;
	std::vector<SnowFlake*> SnowFlakes;

	// For animation
	double CurrentTime = -1.0;
	double Accumulator = 0.0;

	// Lightning flash system
	double LastLightningTime = 0.0;
	double NextLightningTime = 0.0;
	float LightningFlashIntensity = 0.0f;
	int LightningFlashFramesRemaining = 0;

	// Snow wind system
	double LastWindChangeTime = 0.0;
	double NextWindChangeTime = 0.0;
	float CurrentSnowWindDirection = 0.0f;
	float TargetSnowWindDirection = 0.0f;
	float WindTransitionProgress = 1.0f;

	static Setting GeneralSettings;

	DisplayData* pDisplaySpecificData = nullptr;
	MonitorData MonitorDat;

	static LRESULT CALLBACK WndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);

	void InitDirect2D(HWND hWnd);

	void HandleWindowBoundsChange(HWND window, bool clearDrops);
	void HandleTaskBarChange() const;
	void FindSceneRect2(RECT& sceneRect, float& scaleFactor) const;
	void FindSceneRect(RECT& sceneRect, float& scaleFactor) const;

	static void InitNotifyIcon(HWND hWnd);
	static void RemoveNotifyIcon(HWND hWnd);
	static void ShowContextMenu(HWND hWnd);

	static double GetCurrentTimeInSeconds();
	void UpdateRainDrops(float deltaTime);
	void UpdateSnowFlakes(float deltaTime);
	void DrawRainDrops() const;
	void DrawSnowFlakes() const;

	// Lightning flash methods
	void UpdateLightning();
	void DrawLightningFlash() const;

	// Snow wind methods
	void UpdateSnowWind(float deltaTime);
	float GetCurrentSnowWindFactor() const;

	static void SetInstanceToHwnd(HWND hWnd, LPARAM lParam);
	static DisplayWindow* GetInstanceFromHwnd(HWND hWnd);

	// Updates the window position in the z-order to keep it behind active windows
	// but above non-active windows
	static void UpdateZOrder(HWND hWnd);

	// Timer for z-order management
	static constexpr UINT Z_ORDER_TIMER = 1949;
};
