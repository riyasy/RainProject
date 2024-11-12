#pragma once

#include "Vector2.h"
#include "DisplayData.h"

#define SCENE_WIDTH 1920
#define SCENE_HEIGHT 1080
#define TWO_PI 6.28318530718f
#define PI 3.14159265359f
#define SNOW_FLOW_RATE  0.25f

class SnowFlake
{
public:
	SnowFlake(DisplayData* pDispData);
	void UpdatePosition(float deltaSeconds);
	static void SettleSnow();
	void Draw(ID2D1DeviceContext* dc) const;
	static void DrawSettledSnow(ID2D1DeviceContext* dc, const DisplayData* pDispData);

private:
	static constexpr float MAX_SPEED = 75.0f;
	static constexpr float NOISE_INTENSITY = 20.0f;
	static constexpr float NOISE_SCALE = 0.01f;
	static constexpr float NOISE_TIMESCALE = 0.001f;
	static constexpr float GRAVITY = 10.0f;
	static constexpr bool SNOW_COLOR = true;
	static constexpr bool AIR_COLOR = false;

	Vector2 pos;
	Vector2 vel;

	DisplayData* pDisplayData;

	inline static bool scene[SCENE_WIDTH * SCENE_HEIGHT] = {};

	static bool CanSnowFlowInto(int x, int y);
	static bool IsSceneryPixelSet(int x, int y);
	void Spawn();
	void ReSpawn();
};
