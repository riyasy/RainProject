#pragma once

#include <d2d1_3.h>
#include <vector>
#include <dcomp.h>
#include <wrl/client.h>
#include <memory>

class FastNoiseLite;

class DisplayData
{
public:
	explicit DisplayData(ID2D1DeviceContext* dc);
	~DisplayData();
	void SetRainColor(COLORREF color);
	void SetSceneBounds(RECT sceneRect, float scaleFactor);
	void InvalidateSnowAtlas();
	void ClearSnowAccumulation();
	// Switch settle representation: frees the per-pixel buffer in simple mode,
	// (re)allocates it in per-pixel mode, then clears the heap.
	void ApplySnowHeapMode(bool simple);

	int Width = 100;
	int Height = 100;
	float ScaleFactor = 1.0f; // FullHD is considered as 1. 4K will be 2(twice height and width change).

	RECT SceneRect = { 0, 0, 100, 100 };
	RECT SceneRectNorm = { 0, 0, 100, 100 }; // normalized to left top as 0,0

	ID2D1DeviceContext* DC;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	// Single brush reused for all splatter draws — opacity is set dynamically at draw time
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> SplatterColorBrush;

	// Snow flakes are drawn with one batched sprite call: a 2x2 atlas of the
	// four shapes plus a reused sprite batch. Both are device-dependent (rebuilt
	// on device loss); the atlas is also rebuilt on particle-color change.
	Microsoft::WRL::ComPtr<ID2D1Bitmap> SnowAtlas;
	Microsoft::WRL::ComPtr<ID2D1SpriteBatch> SnowSpriteBatch;

	int MaxSnowHeight = 0;
	std::vector<uint8_t> ScenePixels;

	// "Simple snow heap" mode: settled snow as a per-column height (pixels)
	// instead of the per-pixel ScenePixels accumulation — O(Width) memory and
	// per-frame cost. Selected at runtime via the settings checkbox.
	bool SimpleSnowHeap = false;
	std::vector<float> ColumnHeights;
	int SnowColumnWidth = 1; // width in pixels of each ColumnHeights entry (DPI-scaled)

	std::unique_ptr<FastNoiseLite> pNoiseGen;

private:
	// Allocate the per-pixel ScenePixels buffer in per-pixel mode, or free it in
	// simple mode. forceRealloc re-creates it (e.g. after a scene-bounds change).
	void AllocateOrFreeScenePixels(bool forceRealloc);
	static bool IsSame(const RECT& l, const RECT& r);
};