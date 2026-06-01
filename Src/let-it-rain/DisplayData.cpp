#include "DisplayData.h"
#include "FastNoiseLite.h"
#include "SnowFlake.h"
#include <algorithm>
#include <memory>

DisplayData::DisplayData(ID2D1DeviceContext * dc) : DC(dc)
{
	if (pNoiseGen == nullptr)
	{
		pNoiseGen = std::make_unique<FastNoiseLite>();
		pNoiseGen->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	}
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

	// Color changed, so the colored snow atlas is invalid
	InvalidateSnowAtlas();
}

void DisplayData::InvalidateSnowAtlas()
{
	// Atlas is colored with the current particle color; drop it so it is
	// rebuilt on the next snow frame.
	SnowAtlas.Reset();
}

void DisplayData::SetSceneBounds(const RECT sceneRect, const float scaleFactor)
{
	const bool boundsChanged = !IsSame(SceneRect, sceneRect);

	SceneRect = sceneRect;
	ScaleFactor = scaleFactor;

	SceneRectNorm.top = 0;
	SceneRectNorm.left = 0;
	SceneRectNorm.bottom = SceneRect.bottom - SceneRect.top;
	SceneRectNorm.right = SceneRect.right - SceneRect.left;

	Width = SceneRect.right - SceneRect.left;
	Height = SceneRect.bottom - SceneRect.top;

	// Per-column heightmap (simple mode): tiny, sized for every mode. Coarse
	// columns (DPI-scaled) so each settled flake makes a discernible bump.
	if (boundsChanged || ColumnHeights.empty())
	{
		SnowColumnWidth = (std::max)(1, static_cast<int>(SnowFlake::SNOW_COLUMN_WIDTH_BASE * ScaleFactor + 0.5f));
		const int numColumns = (Width + SnowColumnWidth - 1) / SnowColumnWidth;
		ColumnHeights.assign(numColumns, 0.0f);
	}

	// Per-pixel buffer: allocated only in per-pixel mode, freed in simple mode.
	AllocateOrFreeScenePixels(boundsChanged);
}

void DisplayData::AllocateOrFreeScenePixels(const bool forceRealloc)
{
	if (SimpleSnowHeap)
	{
		// Simple (heightmap) mode never touches the per-pixel buffer — release it.
		if (!ScenePixels.empty())
		{
			ScenePixels.clear();
			ScenePixels.shrink_to_fit();
		}
		return;
	}

	// Per-pixel mode: (re)allocate a zeroed buffer on first use or bounds change.
	if (forceRealloc || ScenePixels.empty())
	{
		ScenePixels.assign(static_cast<size_t>(Width) * Height, 0); // zero-initialized
		MaxSnowHeight = Height - 2;
	}
}

void DisplayData::ApplySnowHeapMode(const bool simple)
{
	SimpleSnowHeap = simple;
	AllocateOrFreeScenePixels(false); // free (simple) or allocate (per-pixel)
	ClearSnowAccumulation();          // reset so we never show a half-converted pile
}

void DisplayData::ClearSnowAccumulation()
{
	// Reset both heap representations so switching modes never shows a
	// half-converted pile.
	if (!ScenePixels.empty())
	{
		std::fill(ScenePixels.begin(), ScenePixels.end(), static_cast<uint8_t>(0));
		MaxSnowHeight = Height - 2;
	}
	std::fill(ColumnHeights.begin(), ColumnHeights.end(), 0.0f);
}

bool DisplayData::IsSame(const RECT& l, const RECT& r)
{
	return l.left == r.left && l.top == r.top &&
		l.right == r.right && l.bottom == r.bottom;
}