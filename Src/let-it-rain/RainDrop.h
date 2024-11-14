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

	bool DidTouchGround() const;
	bool IsReadyForErase() const;

	void UpdatePosition(float deltaSeconds);
	void Draw(ID2D1DeviceContext* dc) const;

private:
	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
	static constexpr int MAX_SPLATTER_PER_RAINDROP_ = 3;

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

	std::vector<Splatter*> Splatters;

	void Initialize();
	void CreateSplatters();
};
