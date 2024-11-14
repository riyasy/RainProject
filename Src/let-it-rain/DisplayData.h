#pragma once

#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <wrl/client.h>

class FastNoiseLite;

class DisplayData
{
public:
	explicit DisplayData(ID2D1DeviceContext* dc);
	~DisplayData();
	void SetRainColor(COLORREF color);
	void SetWindowBounds(RECT windowRect, float scaleFactor);
	

	int Width = 100;
	int Height = 100;
	float ScaleFactor = 1.0f; // FullHD is considered as 1. 4K will be 2(twice height and width change).

	RECT WindowRect = {0, 0, 100, 100};
	RECT WindowRectNorm = {0, 0, 100, 100}; // normalized to left top as 0,0

	ID2D1DeviceContext* DC;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;

	// TODO delete the allocated memory	
	int MaxSnowHeight = 0;
	bool* pScenePixels = nullptr;
	FastNoiseLite* pNoiseGen = nullptr;

private:
	static bool IsSame(const RECT& l, const RECT& r);
	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
};
