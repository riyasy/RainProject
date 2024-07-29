#pragma once


#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <vector>
#include <wrl/client.h>

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
	RainDrop(int windowWidth, int windowHeight, int xVelocity, RainDropType type);
	bool DidDropLand() const;
	bool ShouldBeErasedAndDeleted() const;
	void MoveToNewPosition();
	void UpdatePositionAndSpeed(FLOAT x, FLOAT y, float xSpeed, float ySpeed);
	void Draw(ID2D1DeviceContext* dc);
	static void SetRainColor(ID2D1DeviceContext* dc, COLORREF color);
	~RainDrop();

private:
	D2D1_ELLIPSE Ellipse;
	float VelocityX;
	float VelocityY;
	float Gravity;
	float BounceDamping;
	int WindowWidth;
	int WindowHeight;
	RainDropType Type;
	bool LandedDrop = false;
	int SplashCount = 0;
	int FramesForSplatter = 0;

	std::vector<RainDrop*> Splatters;

	static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
	static std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;

	void Initialize();
	void DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const;
	static int GetRandomNumber(int min, int max);
};
