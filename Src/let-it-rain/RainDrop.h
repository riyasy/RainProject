#pragma once

#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <vector>
#include <wrl/client.h>

#include "RandomGenerator.h"

#include "RainWindowData.h"

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

	RainDrop(const int windDirectionFactor, const RainDropType type, RainWindowData* windowData);
	void InitializeSplatter();

	bool DidTouchGround() const;
	bool IsReadyForErase() const;
	void CreateSplatters();
	void MoveToNewPosition();

	void Draw(ID2D1DeviceContext* dc) const;
	void DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const;

	void SetPositionAndSpeed(float x, float y, float xSpeed, float ySpeed);

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

	RainWindowData* pRainWindowData;



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



	

	void InitializeMainDrop();
};
