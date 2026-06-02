#pragma once

#include "Vector2.h"
#include "DisplayData.h"
#include <cstdint>
#include <vector>
#include <d2d1_3.h>

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

	void UpdatePosition(float deltaSeconds, double clockTime);
	static void SettleSnow(DisplayData* pDispData);
	// "Simple snow heap" mode: relax the per-column heightmap (volume-conserving
	// diffusion, matching the macOS build) and draw it as a single filled silhouette.
	static void SmoothSnowHeap(DisplayData* pDispData);
	// Draw all falling flakes in one batched sprite call (atlas + ID2D1SpriteBatch).
	static void DrawFallingFlakes(ID2D1DeviceContext3* dc3, const std::vector<SnowFlake>& flakes, DisplayData* pDispData);
	static void DrawSettledSnow(ID2D1DeviceContext* dc, const DisplayData* pDispData);
	static void DrawSettledSnowSimple(ID2D1DeviceContext* dc, const DisplayData* pDispData);

	// Width (in logical px, DPI-scaled at runtime) of each simple-heap column.
	// Public because DisplayData sizes the ColumnHeights array from it.
	// Matches the macOS build's kSnowColumnSpacing (12 pt).
	// ↑ coarser, chunkier mounds (fewer columns); ↓ finer, smoother slopes (more columns, more CPU).
	static constexpr float SNOW_COLUMN_WIDTH_BASE = 12.0f;

	// Falling-flake count per Intensity unit (snow). Public because
	// DisplayWindow::UpdateSnowFlakes uses it to size the flake pool. Tuned
	// together with SNOW_EDGE_MARGIN to keep on-screen density constant.
	// ↑ denser snowfall per intensity step (more flakes, more CPU); ↓ sparser.
	static constexpr int SNOW_FLAKE_MULTIPLIER = 14;

private:
	// Snowflake shape types
	enum class SnowflakeShape {
		Simple,     // Simple circular shape
		Crystal,    // Star-like crystal shape
		Hexagon,    // Hexagon shape
		Star        // Star shape with more branches
	};
	// Flake speed cap (px/s). ↑ lets flakes move faster (more frantic gusts); ↓ keeps it calm.
	static constexpr float MAX_SPEED = 75.0f;
	// Strength of the noise-driven wind/swirl. ↑ wilder sideways drift & speed
	// variation; ↓ straighter, more uniform fall.
	static constexpr float NOISE_INTENSITY = 20.0f;
	// How fast the noise field evolves over time (the noise's 3rd axis rate).
	// ↑ faster-churning gusts; ↓ slow, lazily-shifting drift.
	static constexpr float NOISE_TIMESCALE = 0.001f;
	// Steady downward pull (accel). ↑ flakes fall faster & straighter; ↓ driftier, hangs longer.
	static constexpr float GRAVITY = 10.0f;
	// Per-pixel-mode scene cell values (not tuning knobs).
	static constexpr uint8_t SNOW_COLOR = 1;
	static constexpr uint8_t AIR_COLOR = 0;
	// Per-pixel-mode settle flow chance (0–10). ↑ snow slumps/flows faster; ↓ stiffer, sticks in place.
	static constexpr int SNOW_FLOW_RATE = 3;

	// Horizontal off-screen spawn/despawn margin as a fraction of scene width
	// (drift headroom for seamless edges). Smaller = fewer off-screen flakes
	// simulated; too small risks flakes popping out at the edges under drift.
	static constexpr float SNOW_EDGE_MARGIN = 0.2f;

	// Flake size as a real radius (matches the macOS build's kSnowMin/MaxRadius),
	// driving both the on-screen draw size and the settled-heap deposit so a flake
	// that looks bigger also settles proportionally more snow.
	// ↑ both = chunkier snow; bigger flakes also settle faster (deposit ∝ radius).
	static constexpr float SNOW_MIN_RADIUS = 0.8f;
	static constexpr float SNOW_MAX_RADIUS = 2.5f;
	// On-screen half-size (px) per unit radius before DPI scaling (macOS kExtent).
	// ↑ visually bigger flakes (same radii); ↓ smaller. Does not affect physics.
	static constexpr float SNOW_DRAW_SCALE = 2.5f;

	// Simple heightmap heap tuning (aligned with the macOS SnowSystem):
	// per-flake deposit = radius * SNOW_DEPOSIT_FACTOR (DPI-scaled).
	// ↑ faster pile buildup per settled flake; ↓ slower.
	static constexpr float SNOW_DEPOSIT_FACTOR = 0.5f;
	// Pile height cap as a fraction of scene height.
	// ↑ lets drifts grow taller before they stop accumulating; ↓ keeps the pile shallow.
	static constexpr float SNOW_MAX_HEIGHT_FRACTION = 0.35f;
	// Volume-conserving slope relaxation: each pass moves SNOW_SMOOTH_RATE of any
	// adjacent height excess above SNOW_SMOOTH_THRESHOLD (px, DPI-scaled) into the
	// lower neighbour. bigger rate = faster smoothing; bigger threshold = steeper.
	static constexpr float SNOW_SMOOTH_RATE = 0.08f;
	static constexpr float SNOW_SMOOTH_THRESHOLD = 2.0f;

	// Resolution for pre-rendered sprites
	static constexpr float SPRITE_SIZE = 64.0f/8;
	// Base size used during pre-rendering to fit within SPRITE_SIZE
	static constexpr float SPRITE_BASE_DRAW_SIZE = 15.0f/8;

	Vector2 Pos;
	Vector2 Vel;
	float Radius;        // Flake radius (logical units; see SNOW_MIN/MAX_RADIUS)
	float Rotation;      // Current rotation angle
	float RotationSpeed; // Speed of rotation
	SnowflakeShape Shape; // Shape type of this snowflake

	DisplayData* pDisplayData;

	static bool CanSnowFlowInto(int x, int y, const DisplayData* pDispData);
	bool IsSceneryPixelSet(int x, int y) const;
	void Spawn();
	void ReSpawn();

	// Build the 2x2 shape atlas into pDispData->SnowAtlas.
	static void GenerateAtlas(ID2D1DeviceContext* dc, DisplayData* pDispData);

	// Helper methods for drawing each shape into the atlas render target.
	static void DrawSimpleSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData);
	static void DrawCrystalSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData);
	static void DrawHexagonSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData);
	static void DrawStarSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData);
};