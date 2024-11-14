#include "DisplayData.h"
#include "FastNoiseLite.h"

DisplayData::DisplayData(ID2D1DeviceContext* dc): DC(dc)
{
	if (pNoiseGen == nullptr)
	{
		pNoiseGen = new FastNoiseLite();
		pNoiseGen->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	}
}

DisplayData::~DisplayData()
{
	delete []pScenePixels;
	delete pNoiseGen;
}

void DisplayData::SetRainColor(const COLORREF color)
{
	const float red = static_cast<float>(GetRValue(color)) / 255.0f;
	const float green = static_cast<float>(GetGValue(color)) / 255.0f;
	const float blue = static_cast<float>(GetBValue(color)) / 255.0f;

	//Main drop is always drawn with opaque brush
	DC->CreateSolidColorBrush(D2D1::ColorF(red, green, blue, 1.0f), DropColorBrush.GetAddressOf());

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
		DC->CreateSolidColorBrush(splatterColor, splatterColorBrush.GetAddressOf());
		PrebuiltSplatterOpacityBrushes.push_back(splatterColorBrush);
	}
}

void DisplayData::SetWindowBounds(const RECT windowRect, const float scaleFactor)
{
	if (!IsSame(WindowRect, windowRect) && pScenePixels != nullptr)
	{
		delete[] pScenePixels;
		pScenePixels = nullptr;
	}

	//wchar_t buffer[100];
	//swprintf_s(buffer, L"Width: %d, Height: %d\n", Width, Height);
	//OutputDebugStringW(buffer);

	WindowRect = windowRect;
	ScaleFactor = scaleFactor;

	WindowRectNorm.top = 0;
	WindowRectNorm.left = 0;
	WindowRectNorm.bottom = WindowRect.bottom - WindowRect.top;
	WindowRectNorm.right = WindowRect.right - WindowRect.left;

	Width = WindowRect.right - WindowRect.left;
	Height = WindowRect.bottom - WindowRect.top;

	if (pScenePixels == nullptr)
	{
		pScenePixels = new bool[Height * Width]();
		MaxSnowHeight = Height - 2;
	}
}

bool DisplayData::IsSame(const RECT& l, const RECT& r)
{
	return l.left == r.left && l.top == r.top &&
		l.right == r.right && l.bottom == r.bottom;
}
