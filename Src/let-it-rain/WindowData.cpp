#include "WindowData.h"

void WindowData::SetRainColor(ID2D1DeviceContext* dc, const COLORREF color)
{
	const float red = static_cast<float>(GetRValue(color)) / 255.0f;
	const float green = static_cast<float>(GetGValue(color)) / 255.0f;
	const float blue = static_cast<float>(GetBValue(color)) / 255.0f;

	//Main drop is always drawn with opaque brush
	dc->CreateSolidColorBrush(D2D1::ColorF(red, green, blue, 1.0f), DropColorBrush.GetAddressOf());

	// Splatter is drawn with transparency increasing as frame count increases
	if (!PrebuiltSplatterOpacityBrushes.empty())
	{
		PrebuiltSplatterOpacityBrushes.clear();
	}
	for (int i = 0; i < MAX_SPLUTTER_FRAME_COUNT_; i++)
	{
		float alpha = 1.0f - static_cast<float>(i) / static_cast<float>(MAX_SPLUTTER_FRAME_COUNT_);
		alpha *= 0.75f;
		const D2D1_COLOR_F splatterColor = D2D1::ColorF(red, green, blue, alpha);
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> splatterColorBrush;
		dc->CreateSolidColorBrush(splatterColor, splatterColorBrush.GetAddressOf());
		PrebuiltSplatterOpacityBrushes.push_back(splatterColorBrush);
	}
}

void WindowData::SetWindowBounds(RECT windowRect, float scaleFactor)
{
	WindowRect = windowRect;
	ScaleFactor = scaleFactor;
}
