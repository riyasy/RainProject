#pragma once
#include <vector>
#include <memory>
#include <array>

#include "DisplayData.h"
#include "Splatter.h"
#include "Vector2.h"

// RainDrop Class
class RainDrop
{
public:
	RainDrop(int windDirectionFactor, DisplayData* pDispData) noexcept;
	~RainDrop() noexcept;

	[[nodiscard]] bool DidTouchGround() const noexcept;
	[[nodiscard]] bool IsReadyForErase() const noexcept;

	void UpdatePosition(float deltaSeconds) noexcept;
	void Draw(ID2D1DeviceContext* dc) const noexcept;

private:
	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
	static constexpr int MAX_SPLATTER_PER_RAINDROP_ = 3;
	static constexpr float PI = 3.14159265359f;

	static constexpr float TERMINAL_VELOCITY_Y = 1000; //pixels per second
	static constexpr float WIND_MULTIPLIER = 75; // pixels per second

	static constexpr float SPLATTER_STARTING_VELOCITY = 200.0f;

	DisplayData* pDisplayData;
	int WindDirectionFactor;

	Vector2 Pos;
	Vector2 Vel;
	float Radius;
	float DropTrailLength;

	bool TouchedGround = false;
	bool IsDead = false;
	int CurrentFrameCountForSplatter = 0;

	std::vector<std::unique_ptr<Splatter>> Splatters;

	void Initialize() noexcept;
	void CreateSplatters() noexcept;
	[[nodiscard]] bool ShouldDrawRainLine(const Vector2& prevPoint) const noexcept;
};
