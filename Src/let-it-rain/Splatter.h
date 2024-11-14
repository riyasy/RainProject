#pragma once

#include "Vector2.h"
#include "DisplayData.h"

// RainDrop Class
class Splatter
{
public:
	Splatter(DisplayData* pDispData, Vector2 pos, Vector2 vel);
	~Splatter();

	void UpdatePosition(float deltaSeconds);
	void Draw(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const;

private:
	static constexpr int MAX_SPLATTER_BOUNCE_COUNT_ = 2;

	static constexpr float GRAVITY = 10.0f; // pixels per second square
	static constexpr float AIR_RESISTANCE = 0.98f; // pixels per second square
	static constexpr float BOUNCE_DAMPING = 0.9f;

	DisplayData* pDisplayData;

	Vector2 Pos;
	Vector2 Vel;
	float Radius;

	int SplatterBounceCount = 0;
};
