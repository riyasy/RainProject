#pragma once
#include <vector>

#include "DisplayData.h"
#include "Splatter.h"
#include "Vector2.h"

// RainDrop Class
class RainDrop
{
public:
	RainDrop(int windDirectionFactor, DisplayData* pDispData);
	~RainDrop();

	// Movable but not copyable (ownership of splatters must transfer)
	RainDrop(RainDrop&& other) noexcept;
	RainDrop& operator=(RainDrop&& other) noexcept;
	RainDrop(const RainDrop&) = delete;
	RainDrop& operator=(const RainDrop&) = delete;

	bool DidTouchGround() const;
	bool IsReadyForErase() const;

	void UpdatePosition(float deltaSeconds);
	void Draw(ID2D1DeviceContext* dc) const;

	// Allow reusing an existing RainDrop object without reallocating
	void Reset(int windDirectionFactor, DisplayData* pDispData);

	// Raindrop count per Intensity unit. Public because
	// DisplayWindow::UpdateRainDrops uses it to size the drop pool.
	// ↑ denser rain per intensity step (more drops, more CPU/GPU); ↓ sparser.
	static constexpr int RAIN_DROP_MULTIPLIER = 3;

private:
	// Splatter burst lifetime in seconds (time-based, frame-rate independent).
	// 0.5 s == the legacy 50-tick count at the fixed 0.01 s step, so the splatter
	// fade is unchanged to an observer.
	// ↑ splash lingers longer before fading out; ↓ vanishes quicker.
	static constexpr float SPLATTER_DURATION_SECONDS = 0.5f;
	// Droplets thrown up per landing. ↑ bushier splash; ↓ sparser.
	static constexpr int MAX_SPLATTER_PER_RAINDROP_ = 3;

	// Drop fall speed (px/s). ↑ faster, straighter rain; ↓ slower, more wind-blown.
	static constexpr float TERMINAL_VELOCITY_Y = 1000;
	// Horizontal wind velocity per wind-direction unit (px/s).
	// ↑ stronger slant at the slider extremes; ↓ gentler.
	static constexpr float WIND_MULTIPLIER = 75;

	// Splatter launch speed (px/s). ↑ higher & wider splash; ↓ smaller pop.
	static constexpr float SPLATTER_STARTING_VELOCITY = 200.0f;

	DisplayData* pDisplayData;
	int WindDirectionFactor;

	Vector2 Pos;
	Vector2 Vel;
	float Radius;

	float DropTrailLength;
	Vector2 TrailDir; // unit travel direction; cached so Draw needs no per-frame sqrt

	bool TouchedGround = false;
	bool IsDead = false;
	float SplatterTime = 0.0f; // seconds since landing; drives the burst fade/expiry

	// Use value semantics for splatters to avoid extra heap allocations
	std::vector<Splatter> Splatters;

	void Initialize();
	void CreateSplatters();
};
