#pragma once

#include "Vector2.h"
#include "DisplayData.h"

#define TWO_PI 6.28318530718f
#define PI 3.14159265359f

class SnowFlake
{
public:
	SnowFlake(DisplayData* pDispData);
	void UpdatePosition(float deltaSeconds);
	static void SettleSnow(const DisplayData* pDispData);
	void Draw(ID2D1DeviceContext* dc) const;
	static void DrawSettledSnow(ID2D1DeviceContext* dc, const DisplayData* pDispData);
	static void DrawSettledSnow2(ID2D1DeviceContext* dc, const DisplayData* pDispData);

private:
	static constexpr float MAX_SPEED = 75.0f;
	static constexpr float NOISE_INTENSITY = 20.0f;
	static constexpr float NOISE_SCALE = 0.01f;
	static constexpr float NOISE_TIMESCALE = 0.001f;
	static constexpr float GRAVITY = 10.0f;
	static constexpr bool SNOW_COLOR = true;
	static constexpr bool AIR_COLOR = false;
	static constexpr int SNOW_FLOW_RATE = 3;

	Vector2 pos;
	Vector2 vel;

	DisplayData* pDisplayData;

	static bool CanSnowFlowInto(int x, int y, const DisplayData* pDispData);
	bool IsSceneryPixelSet(int x, int y) const;
	void Spawn();
	void ReSpawn();
};
