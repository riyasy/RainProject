#include "RainDrop.h"

#include <algorithm>
#include <d2d1.h>
#include <dcomp.h>
#include <wrl/client.h>

#include "MathUtil.h"
#include "RandomGenerator.h"

RainDrop::RainDrop(const int windDirectionFactor, DisplayData* pDispData):
	pDisplayData(pDispData), WindDirectionFactor(windDirectionFactor)
{
	Initialize();
}

// Move constructor
RainDrop::RainDrop(RainDrop&& other) noexcept
{
	pDisplayData = other.pDisplayData;
	WindDirectionFactor = other.WindDirectionFactor;
	Pos = other.Pos;
	Vel = other.Vel;
	Radius = other.Radius;
	DropTrailLength = other.DropTrailLength;
	TouchedGround = other.TouchedGround;
	IsDead = other.IsDead;
	CurrentFrameCountForSplatter = other.CurrentFrameCountForSplatter;
	Splatters = std::move(other.Splatters);

	// Leave other in a safe state
	other.pDisplayData = nullptr;
	other.Splatters.clear();
}

// Move assignment
RainDrop& RainDrop::operator=(RainDrop&& other) noexcept
{
	if (this != &other)
	{
		Splatters.clear();

		pDisplayData = other.pDisplayData;
		WindDirectionFactor = other.WindDirectionFactor;
		Pos = other.Pos;
		Vel = other.Vel;
		Radius = other.Radius;
		DropTrailLength = other.DropTrailLength;
		TouchedGround = other.TouchedGround;
		IsDead = other.IsDead;
		CurrentFrameCountForSplatter = other.CurrentFrameCountForSplatter;
		Splatters = std::move(other.Splatters);

		other.pDisplayData = nullptr;
		other.Splatters.clear();
	}
	return *this;
}

void RainDrop::Reset(int windDirectionFactor, DisplayData* pDispData)
{
	// Reset state so object can be reused
	pDisplayData = pDispData;
	WindDirectionFactor = windDirectionFactor;

	TouchedGround = false;
	IsDead = false;
	CurrentFrameCountForSplatter = 0;

	// Clear any previous splatters (value semantics)
	Splatters.clear();

	Initialize();
}

void RainDrop::Initialize()
{
	// Randomize x position
	const int xWidenToAccountForSlant = pDisplayData->Width / 3;
	Pos.x = static_cast<float>(RandomGenerator::GetInstance().GenerateInt(
		pDisplayData->SceneRect.left - xWidenToAccountForSlant,
		pDisplayData->SceneRect.right + xWidenToAccountForSlant));

	// Randomize y position
	const int y = (RandomGenerator::GetInstance().GenerateInt(pDisplayData->SceneRect.top - pDisplayData->Height / 2,
	                                                          pDisplayData->SceneRect.top) / 10) * 10;
	Pos.y = static_cast<float>(y);

	// Create drop with radius ranging from 0.2 to 0.7 pixels
	Radius = (RandomGenerator::GetInstance().GenerateInt(2, 7) / 10.0f) * pDisplayData->ScaleFactor;

	// Initialize velocity and physics parameters
	Vel.x = WIND_MULTIPLIER * WindDirectionFactor * pDisplayData->ScaleFactor;
	Vel.y = TERMINAL_VELOCITY_Y * pDisplayData->ScaleFactor;

	// Initialize length of the rain drop trail
	DropTrailLength = RandomGenerator::GetInstance().GenerateInt(30, 100) * pDisplayData->ScaleFactor;
}

RainDrop::~RainDrop()
{
	// value-based splatters cleaned automatically
}

bool RainDrop::DidTouchGround() const
{
	return TouchedGround;
}

bool RainDrop::IsReadyForErase() const
{
	return IsDead;
}

void RainDrop::UpdatePosition(const float deltaSeconds)
{
	if (IsDead) return;

	// Update the position of the raindrop
	Pos.x += Vel.x * deltaSeconds;
	Pos.y += Vel.y * deltaSeconds;

	if (!TouchedGround)
	{
		if (Pos.y + Radius >= pDisplayData->SceneRect.bottom)
		{
			TouchedGround = true;
			Pos.y = static_cast<float>(pDisplayData->SceneRect.bottom);

			if (MathUtil::IsPointInRect(pDisplayData->SceneRect, Pos))
			{
				// if the rain touched ground inside bounds, create splatter.
				CreateSplatters();
			}
			else
			{
				IsDead = true;
			}
		}
	}
	else
	{
		for (auto & splatter : Splatters)
		{
			splatter.UpdatePosition(deltaSeconds);
		}
		IsDead = (++CurrentFrameCountForSplatter) >= MAX_SPLUTTER_FRAME_COUNT_;
	}
}

void RainDrop::CreateSplatters()
{
	for (int i = 0; i < MAX_SPLATTER_PER_RAINDROP_; i++)
	{
		const int angleBounce = RandomGenerator::GetInstance().GenerateInt(20, 70, 110, 160);
		const float angleBounceRadians = angleBounce * (3.14f / 180.0f);

		// Calculate velocity components
		const Vector2 velSplatter(SPLATTER_STARTING_VELOCITY * std::cos(angleBounceRadians) * pDisplayData->ScaleFactor,
	                          -SPLATTER_STARTING_VELOCITY * std::sin(angleBounceRadians) * pDisplayData->ScaleFactor);

		Splatters.emplace_back(pDisplayData, Pos, velSplatter);
	}
}

void RainDrop::Draw(ID2D1DeviceContext* dc) const
{
	const Vector2 prevPoint = MathUtil::FindFirstPoint(DropTrailLength, Pos, Vel);

	if (MathUtil::IsPointInRect(pDisplayData->SceneRect, Pos) && MathUtil::IsPointInRect(
		pDisplayData->SceneRect, prevPoint))
	{
		dc->DrawLine(prevPoint.ToD2DPoint(), Pos.ToD2DPoint(), pDisplayData->DropColorBrush.Get(), Radius);
	}
	else if (MathUtil::IsPointInRect(pDisplayData->SceneRect, Pos) || MathUtil::IsPointInRect(
		pDisplayData->SceneRect, prevPoint))
	{
		D2D1_POINT_2F startPoint, endPoint;
		MathUtil::TrimLineSegment(pDisplayData->SceneRect, prevPoint.ToD2DPoint(), Pos.ToD2DPoint(), startPoint,
		                          endPoint);
		dc->DrawLine(startPoint, endPoint, pDisplayData->DropColorBrush.Get(), Radius);
	}

	if (!Splatters.empty())
	{
		for (const auto & splatter : Splatters)
		{
			splatter.Draw(dc, pDisplayData->PrebuiltSplatterOpacityBrushes[CurrentFrameCountForSplatter].Get());
		}
	}
}
