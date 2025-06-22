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

	// Setter for snow accumulation chance
	static void SetSnowAccumulationChance(float chance) {
		s_snowAccumulationChance = chance;
	}

	// Getter for snow accumulation chance
	static float GetSnowAccumulationChance() {
		return s_snowAccumulationChance;
	}

private:
	// Snowflake shape types
	enum class SnowflakeShape {
		Simple,     // Simple circular shape
		Crystal,    // Star-like crystal shape
		Hexagon,    // Hexagon shape
		Star        // Star shape with more branches
	};

	static constexpr float MAX_SPEED = 300.0f; // Increased max speed
	static constexpr float NOISE_INTENSITY = 40.0f; // Doubled noise intensity for more chaotic movement
	static constexpr float NOISE_SCALE = 0.01f;
	static constexpr float NOISE_TIMESCALE = 0.002f; // Increased timescale for faster noise changes
	static constexpr float GRAVITY = 15.0f; // Slightly increased gravity
	static constexpr bool SNOW_COLOR = true;
	static constexpr bool AIR_COLOR = false;
	static constexpr int SNOW_FLOW_RATE = 1; // Reduced from 3 to 1 to slow down settling rate
	static constexpr float MAX_WOBBLE = 1.5f; // Increased wobble for more erratic movement

	// Static member for snow accumulation chance
	static float s_snowAccumulationChance;

	Vector2 Pos;
	Vector2 Vel;
	float Size;          // Size of the snowflake
	float Rotation;      // Current rotation angle
	float RotationSpeed; // Speed of rotation
	float Opacity;       // Transparency value (0.0 - 1.0)
	float WobblePhase;   // Phase for the wobble effect
	float WobbleAmplitude; // Amplitude of the wobble
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
