#include "DisplayData.h"
#include "FastNoiseLite.h"
#include <memory>

DisplayData::DisplayData(ID2D1DeviceContext * dc) : DC(dc)
{
	if (pNoiseGen == nullptr)
	{
		pNoiseGen = std::make_unique<FastNoiseLite>();
		pNoiseGen->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	}
	// Reserve space for the 4 snowflake types
	SpriteCache.resize(4);
}

DisplayData::~DisplayData()
{
	// unique_ptr will clean up automatically
}

void DisplayData::SetRainColor(const COLORREF color)
{
	const float red   = static_cast<float>(GetRValue(color)) / 255.0f;
	const float green = static_cast<float>(GetGValue(color)) / 255.0f;
	const float blue  = static_cast<float>(GetBValue(color)) / 255.0f;

	// Main drop brush — always fully opaque
	DC->CreateSolidColorBrush(D2D1::ColorF(red, green, blue, 1.0f), DropColorBrush.GetAddressOf());

	// Single splatter brush — opacity is changed dynamically at draw time via SetOpacity()
	DC->CreateSolidColorBrush(D2D1::ColorF(red, green, blue, 1.0f), SplatterColorBrush.GetAddressOf());

	// Color changed, so cached sprites are invalid
	InvalidateSpriteCache();
}

void DisplayData::InvalidateSpriteCache()
{
	for (auto& bitmap : SpriteCache)
	{
		bitmap = nullptr;
	}
}

void DisplayData::SetSceneBounds(const RECT sceneRect, const float scaleFactor)
{
	if (!IsSame(SceneRect, sceneRect) && !ScenePixels.empty())
	{
		ScenePixels.clear();
		ScenePixels.shrink_to_fit();
	}

	//wchar_t buffer[100];
	//swprintf_s(buffer, L"Width: %d, Height: %d\n", Width, Height);
	//OutputDebugStringW(buffer);

	SceneRect = sceneRect;
	ScaleFactor = scaleFactor;

	SceneRectNorm.top = 0;
	SceneRectNorm.left = 0;
	SceneRectNorm.bottom = SceneRect.bottom - SceneRect.top;
	SceneRectNorm.right = SceneRect.right - SceneRect.left;

	Width = SceneRect.right - SceneRect.left;
	Height = SceneRect.bottom - SceneRect.top;

	if (ScenePixels.empty())
	{
		ScenePixels.resize(Height * Width);
		// std::vector::resize zero-initializes POD types like uint8_t
		MaxSnowHeight = Height - 2;
	}
}

bool DisplayData::IsSame(const RECT& l, const RECT& r)
{
	return l.left == r.left && l.top == r.top &&
		l.right == r.right && l.bottom == r.bottom;
}