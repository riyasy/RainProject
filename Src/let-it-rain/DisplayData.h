#pragma once

#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <wrl/client.h>

class DisplayData
{
public:
	void SetRainColor(ID2D1DeviceContext* dc, COLORREF color);
	void SetWindowBounds(RECT windowRect, float scaleFactor);
	long GetWidth() const;
	long GetHeight() const;

	float ScaleFactor = 1.0f; // FullHD is considered as 1. 4K will be 2(twice height and width change).
	RECT WindowRect = {0, 0, 100, 100};
	RECT WindowRectNorm = { 0, 0, 100, 100 };


	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;

	// TODO delete the allocated memory
	bool* scene;

private:
	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
};
