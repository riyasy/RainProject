#include "RainDrop.h"
#include <wrl/client.h>
#include <algorithm>

#include "MathUtil.h"

Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> RainDrop::DropColorBrush;
std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> RainDrop::PrebuiltSplatterOpacityBrushes;
float RainDrop::ScaleFactor = 1.0f;
RECT RainDrop::WindowRect;

RainDrop::RainDrop(const int windDirectionFactor, const RainDropType type):
	Type(type), WindDirectionFactor(windDirectionFactor)
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
	// Randomize x position
	int xWidenToAccountForSlant = RECT_WIDTH(WindowRect) / 3;
	Ellipse.point.x = static_cast<float>(RandomGenerator::GetInstance().GenerateInt(
		WindowRect.left - xWidenToAccountForSlant, WindowRect.right + xWidenToAccountForSlant));

	// Randomize y position
	const int y = (RandomGenerator::GetInstance().GenerateInt(-RECT_HEIGHT(WindowRect) / 2, WindowRect.top) / 10) * 10;
	Ellipse.point.y = static_cast<float>(y);

	// Create drop with radius ranging from 0.2 to 0.7 pixels
	Ellipse.radiusX = (RandomGenerator::GetInstance().GenerateInt(2, 7) / 10.0f) * ScaleFactor;
	Ellipse.radiusY = Ellipse.radiusX;

	// Initialize velocity and physics parameters
	DeltaX_PerPhysicsFrame = HORIZONTAL_SPEED_OF_DROP_BASE * PHYSICS_FRAME_INTERVAL * ScaleFactor * WindDirectionFactor;
	DeltaY_PerPhysicsFrame = VERTICAL_SPEED_OF_DROP * PHYSICS_FRAME_INTERVAL * ScaleFactor;

	// Initialize length of the rain drop trail
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
	return IsDead;
}

void RainDrop::MoveToNewPosition()
{
	if (IsDead) return;

	// Update the position of the raindrop
	Ellipse.point.x += DeltaX_PerPhysicsFrame;
	Ellipse.point.y += DeltaY_PerPhysicsFrame;

	if (Type == RainDropType::Splatter)
	{
		// Adjust for Gravity and Air resistance for splatter
		float deltaYAdjustmentDueToGravity = GRAVITY * PHYSICS_FRAME_INTERVAL * ScaleFactor;
		DeltaY_PerPhysicsFrame += deltaYAdjustmentDueToGravity;
		float deltaXAdjustmentDueToAirResistance = AIR_RESISTANCE * PHYSICS_FRAME_INTERVAL * ScaleFactor;

		if (DeltaX_PerPhysicsFrame > 0) // drop going right
		{
			DeltaX_PerPhysicsFrame = (std::max)(0.0f, DeltaX_PerPhysicsFrame - deltaXAdjustmentDueToAirResistance);
		}
		else if (DeltaX_PerPhysicsFrame < 0) // drop moving left
		{
			DeltaX_PerPhysicsFrame = (std::min)(0.0f, DeltaX_PerPhysicsFrame + deltaXAdjustmentDueToAirResistance);
		}
		// Check for bouncing against sides
		if (Ellipse.point.x + Ellipse.radiusX > WindowRect.right || Ellipse.point.x - Ellipse.radiusX < WindowRect.left)
		{
			DeltaX_PerPhysicsFrame = -DeltaX_PerPhysicsFrame;
		}

		// Check for bouncing against bottom border
		if (Ellipse.point.y + Ellipse.radiusY > WindowRect.bottom)
		{
			Ellipse.point.y = WindowRect.bottom - Ellipse.radiusY; // Keep the ellipse within bounds
			DeltaY_PerPhysicsFrame = -DeltaY_PerPhysicsFrame * BOUNCE_DAMPING; // Bounce with damping
			SplatterBounceCount++;
		}
		// Check for bouncing against top
		if (Ellipse.point.y - Ellipse.radiusY < WindowRect.top)
		{
			Ellipse.point.y = Ellipse.radiusY; // Keep the ellipse within bounds
			DeltaY_PerPhysicsFrame = -DeltaY_PerPhysicsFrame; // Reverse the direction if it hits the top edge
		}
	}

	if (Type == RainDropType::MainDrop)
	{
		if (!TouchedGround)
		{
			if (Ellipse.point.y + Ellipse.radiusY >= WindowRect.bottom)
			{
				TouchedGround = true;
				Ellipse.point.y = static_cast<float>(WindowRect.bottom);

				if (MathUtil::IsPointInRect(WindowRect, Ellipse.point))
				{
					// if the rain touched ground inside bounds, create splatter.
					CreateSplatters();
				}
				else
				{
					// else mark for cleanup.
					IsDead = true;
				}
			}
		}
		else
		{
			for (const auto drop : Splatters)
			{
				drop->MoveToNewPosition();
			}
			IsDead = (++CurrentFrameCountForSplatter) >= MAX_SPLUTTER_FRAME_COUNT_;
		}
	}
}

void RainDrop::CreateSplatters()
{
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

void RainDrop::Draw(ID2D1DeviceContext* dc) const
{
	if (Type == RainDropType::MainDrop && !TouchedGround)
	{
		D2D1_POINT_2F prevPoint;
		prevPoint.x = Ellipse.point.x - DropTrailLength * DeltaX_PerPhysicsFrame;
		prevPoint.y = Ellipse.point.y - DropTrailLength * DeltaY_PerPhysicsFrame;

		if (MathUtil::IsPointInRect(WindowRect, Ellipse.point) && MathUtil::IsPointInRect(WindowRect, prevPoint))
		{
			dc->DrawLine(prevPoint, Ellipse.point, DropColorBrush.Get(), Ellipse.radiusX);
		}
		else if (MathUtil::IsPointInRect(WindowRect, Ellipse.point) || MathUtil::IsPointInRect(WindowRect, prevPoint))
		{
			D2D1_POINT_2F adjustedPoint1, adjustedPoint2;
			MathUtil::TrimLineSegment(WindowRect, prevPoint, Ellipse.point, adjustedPoint1, adjustedPoint2);
			dc->DrawLine(adjustedPoint1, adjustedPoint2, DropColorBrush.Get(), Ellipse.radiusX);
		}
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
		MathUtil::IsPointInRect(WindowRect, Ellipse.point) &&
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
	float red = static_cast<float>(GetRValue(color)) / 255.0f;
	float green = static_cast<float>(GetGValue(color)) / 255.0f;
	float blue = static_cast<float>(GetBValue(color)) / 255.0f;

	//Main drop is always drawn with opaque brush
	dc->CreateSolidColorBrush(D2D1::ColorF(red, green, blue, 1.0f), DropColorBrush.GetAddressOf());

	// Splatter is drawn with transparency increasing as frame count increases
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

void RainDrop::SetWindowBounds(RECT windowRect, float scaleFactor)
{
	WindowRect = windowRect;
	ScaleFactor = scaleFactor;
}
