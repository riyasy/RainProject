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
	
	// Apply wind to the snowflake's velocity
	void ApplyWind(float windFactor, float deltaTime);

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

	static constexpr float MAX_SPEED = 250.0f; // Increased max speed for more dynamic movement
	static constexpr float NOISE_INTENSITY = 30.0f; // Maintained noise intensity from previous changes
	static constexpr float NOISE_SCALE = 0.005f; // Maintained noise scale from previous changes
	static constexpr float NOISE_TIMESCALE = 0.1f; // Maintained time scale from previous changes
	static constexpr float GRAVITY = 12.0f; // Maintained gravity from previous changes
	static constexpr bool SNOW_COLOR = true;
	static constexpr bool AIR_COLOR = false;
	static constexpr int SNOW_FLOW_RATE = 1; // Maintained flow rate from previous changes
	static constexpr float MAX_WOBBLE = 1.2f; // Increased max wobble for more visible wind effects
	static constexpr float WIND_RESISTANCE = 0.25f; // Increased wind resistance for more visible effects
	static constexpr float MAX_WIND_SPEED = 80.0f; // Significantly increased max wind speed for more visible effects

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
	float WindResistance;  // Individual wind resistance for this snowflake
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
