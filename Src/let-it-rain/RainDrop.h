#pragma once

#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <vector>
#include <wrl/client.h>

#include "RandomGenerator.h"

// Enum for RainDrop types
enum class RainDropType
{
	MainDrop,
	Splatter
};

// RainDrop Class
class RainDrop
{
public:
	static constexpr float PHYSICS_FRAME_INTERVAL = 0.01f; // in second

	RainDrop(int windDirectionFactor, RainDropType type);
	void InitializeSplatter();

	bool DidTouchGround() const;
	bool IsReadyForErase() const;
	void CreateSplatters();
	void MoveToNewPosition();

	void Draw(ID2D1DeviceContext* dc) const;
	void DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const;

	void SetPositionAndSpeed(float x, float y, float xSpeed, float ySpeed);
	static void SetRainColor(ID2D1DeviceContext* dc, COLORREF color);
	static void SetWindowBounds(RECT windowRect, float scaleFactor);
	~RainDrop();

private:
	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
	static constexpr int MAX_SPLATTER_BOUNCE_COUNT_ = 2;
	static constexpr int MAX_SPLATTER_PER_RAINDROP_ = 3;

	static constexpr float VERTICAL_SPEED_OF_DROP = 1500; //pixels per second
	static constexpr float HORIZONTAL_SPEED_OF_DROP_BASE = 100; // pixels per second
	static constexpr float GRAVITY = 20.0f; // pixels per second square
	static constexpr float AIR_RESISTANCE = 1.0f; // pixels per second square
	static constexpr float BOUNCE_DAMPING = 0.9f;

	//static int WindowWidth; //  in pixels
	//static int WindowHeight; //  in pixels
	static RECT WindowRect;
	static float ScaleFactor; // FullHD is considered as 1. 4K will be 2(twice height and width change).
	static float Gravity;

	RainDropType Type;
	int WindDirectionFactor;
	D2D1_ELLIPSE Ellipse;
	float DeltaX_PerPhysicsFrame;
	float DeltaY_PerPhysicsFrame;
	int DropTrailLength;

	bool TouchedGround = false;
	bool IsDead = false;
	int SplatterBounceCount = 0;
	int CurrentFrameCountForSplatter = 0;

	std::vector<RainDrop*> Splatters;

	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	static std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;
	//static Microsoft::WRL::ComPtr < ID2D1StrokeStyle1> strokeStyleFixedThickness;	

	void InitializeMainDrop();
};
