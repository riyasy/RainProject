#pragma once

#include "Vector2.h"
#include "DisplayData.h"

// Splatter Class
class Splatter
{
public:
	Splatter(DisplayData* pDispData, Vector2 pos, Vector2 vel);
	~Splatter();

	// Default move/copy are fine (no owning heap resources)
	Splatter(const Splatter&) = default;
	Splatter& operator=(const Splatter&) = default;
	Splatter(Splatter&&) = default;
	Splatter& operator=(Splatter&&) = default;

	void UpdatePosition(float deltaSeconds);
	void Draw(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const;

private:
	// Bounces before a splatter stops drawing. ↑ keeps bouncing longer; ↓ settles sooner.
	static constexpr int MAX_SPLATTER_BOUNCE_COUNT_ = 2;

	// Time-based forces (per second), integrated with deltaSeconds each step so
	// the splatter is frame-rate independent. Values are derived from the legacy
	// per-step constants at the fixed 0.01 s step so behaviour is unchanged:
	//   GRAVITY * 0.01      == legacy "+10 per step"
	//   1 - AIR_DAMP * 0.01 == legacy "*0.98 per step"
	// gravity on splatters (px/s^2). ↑ lower, snappier arcs; ↓ floatier, taller pops.
	static constexpr float GRAVITY = 1000.0f;
	// horizontal speed bleed-off rate (per s). ↑ loses sideways drift sooner; ↓ skates farther.
	static constexpr float AIR_DAMP = 2.0f;
	// fraction of vertical speed kept per floor bounce (0–1).
	// ↑ toward 1 = bouncier, taller rebounds; ↓ toward 0 = dead, no bounce.
	static constexpr float BOUNCE_DAMPING = 0.9f;

	DisplayData* pDisplayData; // not owned

	Vector2 Pos;
	Vector2 Vel;
	float Radius;

	int SplatterBounceCount = 0;
};
