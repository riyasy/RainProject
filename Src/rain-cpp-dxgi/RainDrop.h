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
	bool DidDropLand() const;
	bool ShouldBeErasedAndDeleted() const;
	void MoveToNewPosition();
	void UpdatePositionAndSpeed(float x, float y, float xSpeed, float ySpeed);
	void Draw(ID2D1DeviceContext* dc);
	static void SetRainColor(ID2D1DeviceContext* dc, COLORREF color);
	static void SetWindowBounds(int windowWidth, int windowHeight, float scaleFactor);
	~RainDrop();

private:
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


	bool LandedDrop = false;
	int SplashCount = 0;
	int CurrentFrameCountForSplatter = 0;

	std::vector<RainDrop*> Splatters;

	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;

	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	static std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;
	//static Microsoft::WRL::ComPtr < ID2D1StrokeStyle1> strokeStyleFixedThickness;	

	void Initialize();
	void DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const;
};
