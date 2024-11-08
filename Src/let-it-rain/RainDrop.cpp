#include "RainDrop.h"
#include <wrl/client.h>
#include <algorithm>

#include "MathUtil.h"

RainDrop::RainDrop(const int windDirectionFactor, const RainDropType type, WindowData* windowData):
	pWindowData(windowData), Type(type), WindDirectionFactor(windDirectionFactor)
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
	const int xWidenToAccountForSlant = RECT_WIDTH(pWindowData->WindowRect) / 3;
	Ellipse.point.x = static_cast<float>(RandomGenerator::GetInstance().GenerateInt(
		pWindowData->WindowRect.left - xWidenToAccountForSlant,
		pWindowData->WindowRect.right + xWidenToAccountForSlant));

	// Randomize y position
	const int y = (RandomGenerator::GetInstance().GenerateInt(-RECT_HEIGHT(pWindowData->WindowRect) / 2,
	                                                          pWindowData->WindowRect.top) / 10) * 10;
	Ellipse.point.y = static_cast<float>(y);

	// Create drop with radius ranging from 0.2 to 0.7 pixels
	Ellipse.radiusX = (RandomGenerator::GetInstance().GenerateInt(2, 7) / 10.0f) * pWindowData->ScaleFactor;
	Ellipse.radiusY = Ellipse.radiusX;

	// Initialize velocity and physics parameters
	DeltaX_PerPhysicsFrame = HORIZONTAL_SPEED_OF_DROP_BASE * PHYSICS_FRAME_INTERVAL * pWindowData->ScaleFactor *
		WindDirectionFactor;
	DeltaY_PerPhysicsFrame = VERTICAL_SPEED_OF_DROP * PHYSICS_FRAME_INTERVAL * pWindowData->ScaleFactor;

	// Initialize length of the rain drop trail
	DropTrailLength = RandomGenerator::GetInstance().GenerateInt(2, 6);
}

void RainDrop::InitializeSplatter()
{
	// Create splatters with radius ranging from 1.0 to 2.0 pixels
	Ellipse.radiusX = (RandomGenerator::GetInstance().GenerateInt(15, 25) / 10.0f) * pWindowData->ScaleFactor;
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
		const float deltaYAdjustmentDueToGravity = GRAVITY * PHYSICS_FRAME_INTERVAL * pWindowData->ScaleFactor;
		DeltaY_PerPhysicsFrame += deltaYAdjustmentDueToGravity;
		const float deltaXAdjustmentDueToAirResistance = AIR_RESISTANCE * PHYSICS_FRAME_INTERVAL * pWindowData->
			ScaleFactor;

		if (DeltaX_PerPhysicsFrame > 0) // drop going right
		{
			DeltaX_PerPhysicsFrame = (std::max)(0.0f, DeltaX_PerPhysicsFrame - deltaXAdjustmentDueToAirResistance);
		}
		else if (DeltaX_PerPhysicsFrame < 0) // drop moving left
		{
			DeltaX_PerPhysicsFrame = (std::min)(0.0f, DeltaX_PerPhysicsFrame + deltaXAdjustmentDueToAirResistance);
		}
		// Check for bouncing against sides
		if (Ellipse.point.x + Ellipse.radiusX > pWindowData->WindowRect.right || Ellipse.point.x - Ellipse.radiusX <
			pWindowData->WindowRect.left)
		{
			DeltaX_PerPhysicsFrame = -DeltaX_PerPhysicsFrame;
		}

		// Check for bouncing against bottom border
		if (Ellipse.point.y + Ellipse.radiusY > pWindowData->WindowRect.bottom)
		{
			Ellipse.point.y = pWindowData->WindowRect.bottom - Ellipse.radiusY; // Keep the ellipse within bounds
			DeltaY_PerPhysicsFrame = -DeltaY_PerPhysicsFrame * BOUNCE_DAMPING; // Bounce with damping
			SplatterBounceCount++;
		}
		// Check for bouncing against top
		if (Ellipse.point.y - Ellipse.radiusY < pWindowData->WindowRect.top)
		{
			Ellipse.point.y = Ellipse.radiusY; // Keep the ellipse within bounds
			DeltaY_PerPhysicsFrame = -DeltaY_PerPhysicsFrame; // Reverse the direction if it hits the top edge
		}
	}

	if (Type == RainDropType::MainDrop)
	{
		if (!TouchedGround)
		{
			if (Ellipse.point.y + Ellipse.radiusY >= pWindowData->WindowRect.bottom)
			{
				TouchedGround = true;
				Ellipse.point.y = static_cast<float>(pWindowData->WindowRect.bottom);

				if (MathUtil::IsPointInRect(pWindowData->WindowRect, Ellipse.point))
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
		auto splatter = new RainDrop(0, RainDropType::Splatter, pWindowData);

		const float randomizedAndBiasedWindDirectionForSplatter = RandomGenerator::GetInstance().GenerateInt(
			-15 + WindDirectionFactor, -6, 6, 15 + WindDirectionFactor);

		float xVelocityForSplatter = HORIZONTAL_SPEED_OF_DROP_BASE * PHYSICS_FRAME_INTERVAL *
			randomizedAndBiasedWindDirectionForSplatter;
		xVelocityForSplatter = xVelocityForSplatter / 10.0f;
		float yVelocityForSplatter = -std::fabs(2.5f / xVelocityForSplatter);

		xVelocityForSplatter *= pWindowData->ScaleFactor;
		yVelocityForSplatter *= pWindowData->ScaleFactor;

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

		if (MathUtil::IsPointInRect(pWindowData->WindowRect, Ellipse.point) && MathUtil::IsPointInRect(
			pWindowData->WindowRect, prevPoint))
		{
			dc->DrawLine(prevPoint, Ellipse.point, pWindowData->DropColorBrush.Get(), Ellipse.radiusX);
		}
		else if (MathUtil::IsPointInRect(pWindowData->WindowRect, Ellipse.point) || MathUtil::IsPointInRect(
			pWindowData->WindowRect, prevPoint))
		{
			D2D1_POINT_2F adjustedPoint1, adjustedPoint2;
			MathUtil::TrimLineSegment(pWindowData->WindowRect, prevPoint, Ellipse.point, adjustedPoint1,
			                          adjustedPoint2);
			dc->DrawLine(adjustedPoint1, adjustedPoint2, pWindowData->DropColorBrush.Get(), Ellipse.radiusX);
		}
	}
	if (!Splatters.empty())
	{
		for (const auto drop : Splatters)
		{
			drop->DrawSplatter(dc, pWindowData->PrebuiltSplatterOpacityBrushes[CurrentFrameCountForSplatter].Get());
		}
	}
}

void RainDrop::DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const
{
	if (Type == RainDropType::Splatter &&
		MathUtil::IsPointInRect(pWindowData->WindowRect, Ellipse.point) &&
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
