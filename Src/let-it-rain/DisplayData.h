#pragma once

#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <wrl/client.h>

class DisplayData
{
public:
	DisplayData(ID2D1DeviceContext* dc);
	void SetRainColor(COLORREF color);
	void SetWindowBounds(RECT windowRect, float scaleFactor);

	long Width = 100;
	long Height = 100;
	float ScaleFactor = 1.0f; // FullHD is considered as 1. 4K will be 2(twice height and width change).

	RECT WindowRect = {0, 0, 100, 100};
	RECT WindowRectNorm = {0, 0, 100, 100}; // normalized to left top as 0,0

	ID2D1DeviceContext* DC;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;

	// TODO delete the allocated memory
	bool* scene = nullptr;
	int maxSnowHeight = 0;
	// for optimizing settled snow draw. The draw loop has to execute from bottom until this value only.

private:
	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
};
