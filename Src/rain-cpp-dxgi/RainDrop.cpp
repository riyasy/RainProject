#include "RainDrop.h"
#include <wrl/client.h>
#include <algorithm>


Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> RainDrop::DropColorBrush;
std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> RainDrop::PrebuiltSplatterOpacityBrushes;
//Microsoft::WRL::ComPtr < ID2D1StrokeStyle1> RainDrop::strokeStyleFixedThickness;
float RainDrop::ScaleFactor = 1.0f;
int RainDrop::WindowWidth;
int RainDrop::WindowHeight;

RainDrop::RainDrop(const int windDirectionFactor,
                   const RainDropType type):
	Type(type),
	WindDirectionFactor(windDirectionFactor)

{
	if (Type == RainDropType::MainDrop)
	{
		InitializeMainDrop();
	}
	else
	{
		InitializeSplatter();
	}
}


void RainDrop::InitializeMainDrop()
{
	int xWidenToAccountForSlant = WindowHeight / 3;
	Ellipse.point.x = static_cast<float>(RandomGenerator::GetInstance().GenerateInt(
		-xWidenToAccountForSlant, WindowWidth + xWidenToAccountForSlant));

	const int y = (RandomGenerator::GetInstance().GenerateInt(-WindowHeight / 2, 0) / 10) * 10;
	Ellipse.point.y = static_cast<float>(y);


	// Create splatters with radius ranging from 0.2 to 0.7 pixels
	Ellipse.radiusX = (RandomGenerator::GetInstance().GenerateInt(2, 7) / 10.0f) * ScaleFactor;
	Ellipse.radiusY = Ellipse.radiusX;


	// Initialize velocity and physics parameters
	DeltaX_PerPhysicsFrame = HORIZONTAL_SPEED_OF_DROP_BASE * PHYSICS_FRAME_INTERVAL * ScaleFactor * WindDirectionFactor;
	DeltaY_PerPhysicsFrame = VERTICAL_SPEED_OF_DROP * PHYSICS_FRAME_INTERVAL * ScaleFactor;

	DropTrailLength = RandomGenerator::GetInstance().GenerateInt(2, 6);
}

void RainDrop::InitializeSplatter()
{
	// Create splatters with radius ranging from 1.0 to 2.0 pixels
	Ellipse.radiusX = (RandomGenerator::GetInstance().GenerateInt(15, 25) / 10.0f) * ScaleFactor;
	Ellipse.radiusY = Ellipse.radiusX;
}

RainDrop::~RainDrop()
{
	for (const auto drop : Splatters)
	{
		delete drop;
	}
}

bool RainDrop::DidTouchGround() const
{
	return TouchedGround;
}

bool RainDrop::IsReadyForErase() const
{
	return CurrentFrameCountForSplatter >= MAX_SPLUTTER_FRAME_COUNT_;
}

void RainDrop::MoveToNewPosition()
{
	// Update the position of the raindrop
	Ellipse.point.x += DeltaX_PerPhysicsFrame;
	Ellipse.point.y += DeltaY_PerPhysicsFrame;


	if (Type == RainDropType::Splatter)
	{
		// Adjust for Gravity and Air resistance for splatter
		float deltaYAdjustmentDueToGravity = GRAVITY * PHYSICS_FRAME_INTERVAL * ScaleFactor;
		DeltaY_PerPhysicsFrame += deltaYAdjustmentDueToGravity;

		float deltaXAdjustmentDueToAirResistance = AIR_RESISTANCE * PHYSICS_FRAME_INTERVAL * ScaleFactor;
		if (DeltaX_PerPhysicsFrame > 0)
		{
			DeltaX_PerPhysicsFrame = (std::max)(0.0f, DeltaX_PerPhysicsFrame - deltaXAdjustmentDueToAirResistance);
		}
		else if (DeltaX_PerPhysicsFrame < 0)
		{
			DeltaX_PerPhysicsFrame = (std::min)(0.0f, DeltaX_PerPhysicsFrame + deltaXAdjustmentDueToAirResistance);
		}


		// Check for Bouncing against ground and window left/right
		if (Ellipse.point.x + Ellipse.radiusX > WindowWidth || Ellipse.point.x - Ellipse.radiusX < 0)
		{
			DeltaX_PerPhysicsFrame = -DeltaX_PerPhysicsFrame;
		}

		if (Ellipse.point.y + Ellipse.radiusY > WindowHeight)
		{
			Ellipse.point.y = WindowHeight - Ellipse.radiusY; // Keep the ellipse within bounds
			DeltaY_PerPhysicsFrame = -DeltaY_PerPhysicsFrame * BOUNCE_DAMPING; // Bounce with damping
			SplatterBounceCount++;
		}

		if (Ellipse.point.y - Ellipse.radiusY < 0)
		{
			Ellipse.point.y = Ellipse.radiusY; // Keep the ellipse within bounds
			DeltaY_PerPhysicsFrame = -DeltaY_PerPhysicsFrame; // Reverse the direction if it hits the top edge
		}
	}


	if (Type == RainDropType::MainDrop)
	{
		if (!TouchedGround)
		{
			if (Ellipse.point.y + Ellipse.radiusY >= WindowHeight)
			{
				Ellipse.point.y = static_cast<float>(WindowHeight);
				TouchedGround = true;
				for (int i = 0; i < MAX_SPLATTER_PER_RAINDROP_; i++)
				{
					auto splatter = new RainDrop(0, RainDropType::Splatter);

					float randomizedAndBiasedWindDirectionForSplatter = RandomGenerator::GetInstance().GenerateInt(
						-15 + WindDirectionFactor, -6, 6, 15 + WindDirectionFactor);

					float xVelocityForSplatter = HORIZONTAL_SPEED_OF_DROP_BASE * PHYSICS_FRAME_INTERVAL *
						randomizedAndBiasedWindDirectionForSplatter;
					xVelocityForSplatter = xVelocityForSplatter / 10.0f;
					float yVelocityForSplatter = -std::fabs(2.5f / xVelocityForSplatter);

					xVelocityForSplatter *= ScaleFactor;
					yVelocityForSplatter *= ScaleFactor;

					splatter->SetPositionAndSpeed(Ellipse.point.x, Ellipse.point.y - splatter->Ellipse.radiusX,
					                              xVelocityForSplatter, yVelocityForSplatter);
					Splatters.push_back(splatter);
				}
			}
		}
		else
		{
			for (const auto drop : Splatters)
			{
				drop->MoveToNewPosition();
			}
			CurrentFrameCountForSplatter++;
		}
	}
}


void RainDrop::Draw(ID2D1DeviceContext* dc) const
{
	//D2D1_RECT_F rect;
	//rect.top = 0;
	//rect.left = 0;
	//rect.right = WindowWidth;
	//rect.bottom = WindowHeight;
	//dc->DrawRectangle(&rect, DropColorBrush.Get());


	if (Type == RainDropType::MainDrop && !TouchedGround &&
		Ellipse.point.y >= 0 &&
		Ellipse.point.y <= WindowHeight &&
		Ellipse.point.x >= 0 &&
		Ellipse.point.x <= WindowWidth)
	{
		//dc->FillEllipse(ellipse, pBrush);
		D2D1_POINT_2F prevPoint;
		prevPoint.x = Ellipse.point.x - DropTrailLength * DeltaX_PerPhysicsFrame;
		prevPoint.y = Ellipse.point.y - DropTrailLength * DeltaY_PerPhysicsFrame;
		dc->DrawLine(prevPoint, Ellipse.point, DropColorBrush.Get(), Ellipse.radiusX);
	}
	if (!Splatters.empty())
	{
		for (const auto drop : Splatters)
		{
			drop->DrawSplatter(dc, PrebuiltSplatterOpacityBrushes[CurrentFrameCountForSplatter].Get());
		}
	}
}

void RainDrop::DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const
{
	if (Type == RainDropType::Splatter &&
		Ellipse.point.y >= 0 && Ellipse.point.x >= 0 &&
		SplatterBounceCount < MAX_SPLATTER_BOUNCE_COUNT_)
	{
		dc->FillEllipse(Ellipse, pBrush);
	}
}


void RainDrop::SetPositionAndSpeed(const FLOAT x, const FLOAT y, const float xSpeed, const float ySpeed)
{
	Ellipse.point.x = x;
	Ellipse.point.y = y;
	DeltaX_PerPhysicsFrame = xSpeed;
	DeltaY_PerPhysicsFrame = ySpeed;
}

void RainDrop::SetRainColor(ID2D1DeviceContext* dc, const COLORREF color)
{
	// Extract the individual color components from COLORREF
	float red = static_cast<float>(GetRValue(color)) / 255.0f;
	float green = static_cast<float>(GetGValue(color)) / 255.0f;
	float blue = static_cast<float>(GetBValue(color)) / 255.0f;

	dc->CreateSolidColorBrush(D2D1::ColorF(red, green, blue, 1.0f), DropColorBrush.GetAddressOf());

	if (!PrebuiltSplatterOpacityBrushes.empty())
	{
		PrebuiltSplatterOpacityBrushes.clear();
	}
	for (int i = 0; i < MAX_SPLUTTER_FRAME_COUNT_; i++)
	{
		float alpha = 1.0f - static_cast<float>(i) / static_cast<float>(MAX_SPLUTTER_FRAME_COUNT_);
		alpha *= 0.75f;
		const D2D1_COLOR_F splatterColor = D2D1::ColorF(red, green, blue, alpha);
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> splatterColorBrush;
		dc->CreateSolidColorBrush(splatterColor, splatterColorBrush.GetAddressOf());
		PrebuiltSplatterOpacityBrushes.push_back(splatterColorBrush);
	}
}

//void RainDrop::SetRainColor(ID2D1Factory2* factory, ID2D1DeviceContext* dc, COLORREF color)
//{
//	SetRainColor(dc, color);
//
//	HRESULT hr = factory->CreateStrokeStyle(
//		D2D1::StrokeStyleProperties1(
//			D2D1_CAP_STYLE_FLAT,
//			D2D1_CAP_STYLE_FLAT,
//			D2D1_CAP_STYLE_FLAT,
//			D2D1_LINE_JOIN_MITER,
//			10.0f,
//			D2D1_DASH_STYLE_SOLID,
//			0.0f, D2D1_STROKE_TRANSFORM_TYPE_FIXED),
//		nullptr, 0, strokeStyleFixedThickness.GetAddressOf());
//}

void RainDrop::SetWindowBounds(int windowWidth, int windowHeight, float scaleFactor)
{
	WindowWidth = windowWidth;
	WindowHeight = windowHeight;
	ScaleFactor = scaleFactor;
}
