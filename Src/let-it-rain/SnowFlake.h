#pragma once

#include "Vector2.h"
#include "DisplayData.h"
#include <cstdint>

#define TWO_PI 6.28318530718f
#define PI 3.14159265359f

class SnowFlake
{
public:
	SnowFlake(DisplayData* pDispData);

	// Movable but not copyable
	SnowFlake(SnowFlake&& other) noexcept;
	SnowFlake& operator=(SnowFlake&& other) noexcept;
	SnowFlake(const SnowFlake&) = delete;
	SnowFlake& operator=(const SnowFlake&) = delete;

	void UpdatePosition(float deltaSeconds);
	static void SettleSnow(DisplayData* pDispData);
	void Draw(ID2D1DeviceContext* dc) const;
	static void DrawSettledSnow(ID2D1DeviceContext* dc, const DisplayData* pDispData);
	static void DrawSettledSnow2(ID2D1DeviceContext* dc, const DisplayData* pDispData);
	

private:
	// Snowflake shape types
	enum class SnowflakeShape {
		Simple,     // Simple circular shape
		Crystal,    // Star-like crystal shape
		Hexagon,    // Hexagon shape
		Star        // Star shape with more branches
	};
	static constexpr float MAX_SPEED = 75.0f;
	static constexpr float NOISE_INTENSITY = 20.0f;
	static constexpr float NOISE_SCALE = 0.01f;
	static constexpr float NOISE_TIMESCALE = 0.001f;
	static constexpr float GRAVITY = 10.0f;
	static constexpr uint8_t SNOW_COLOR = 1;
	static constexpr uint8_t AIR_COLOR = 0;
	static constexpr int SNOW_FLOW_RATE = 3;

	Vector2 Pos;
	Vector2 Vel;
	float Size;          // Size of the snowflake
	float Rotation;      // Current rotation angle
	float RotationSpeed; // Speed of rotation
	SnowflakeShape Shape; // Shape type of this snowflake

	DisplayData* pDisplayData;

	static bool CanSnowFlowInto(int x, int y, const DisplayData* pDispData);
	bool IsSceneryPixelSet(int x, int y) const;
	void Spawn();
	void ReSpawn();	
	// Helper methods for drawing different snowflake shapes
	void DrawSimpleSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const;
	void DrawCrystalSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const;
	void DrawHexagonSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const;
	void DrawStarSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const;
};
