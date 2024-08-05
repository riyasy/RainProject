#pragma once


#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <vector>
#include <wrl/client.h>

#include "RandomGenerator.h"

// Enum for RainDrop types
enum class RainDropType
{
	MainDrop,
	Splatter
};

// RainDrop Class
class RainDrop
{
public:
	RainDrop(int xVelocity, RainDropType type);

	bool DidTouchGround() const;
	bool IsReadyForErase() const;
	void MoveToNewPosition();
	void Draw(ID2D1DeviceContext* dc);
	void DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const;

	void SetPositionAndSpeed(float x, float y, float xSpeed, float ySpeed);
	static void SetRainColor(ID2D1DeviceContext* dc, COLORREF color);
	static void SetWindowBounds(int windowWidth, int windowHeight, float scaleFactor);
	~RainDrop();

private:
	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
	static constexpr int MAX_SPLATTER_BOUNCE_COUNT_ = 3;
	static constexpr int MAX_SPLATTER_PER_RAINDROP_ = 3;

	D2D1_ELLIPSE Ellipse;
	float VelocityX;
	float VelocityY;
	int dropTrailFactor;
	RainDropType Type;

	static float Gravity;
	static float BounceDamping;
	static int WindowWidth;
	static int WindowHeight;
	static float ScaleFactor;


	bool TouchedGround = false;
	int SplatterBounceCount = 0;
	int CurrentFrameCountForSplatter = 0;

	std::vector<RainDrop*> Splatters;

	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	static std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;
	//static Microsoft::WRL::ComPtr < ID2D1StrokeStyle1> strokeStyleFixedThickness;	

	void Initialize();
};
