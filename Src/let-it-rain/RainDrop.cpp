#include "RainDrop.h"

#include <algorithm>
#include <cmath>
#include <d2d1.h>
#include <dcomp.h>
#include <wrl/client.h>

#include "MathUtil.h"
#include "RandomGenerator.h"

RainDrop::RainDrop(const int windDirectionFactor, DisplayData* pDispData) noexcept
	: pDisplayData(pDispData), WindDirectionFactor(windDirectionFactor)
{
	Splatters.reserve(MAX_SPLATTER_PER_RAINDROP_);
	Initialize();
}

void RainDrop::Initialize() noexcept
{
	// Randomize x position with wind compensation
	const int xWidenToAccountForSlant = pDisplayData->Width / 3;
	Pos.x = static_cast<float>(RandomGenerator::GetInstance().GenerateInt(
		pDisplayData->SceneRect.left - xWidenToAccountForSlant,
		pDisplayData->SceneRect.right + xWidenToAccountForSlant));

	// Randomize y position (quantized to improve performance)
	const int y = (RandomGenerator::GetInstance().GenerateInt(
		pDisplayData->SceneRect.top - pDisplayData->Height / 2,
		pDisplayData->SceneRect.top) / 10) * 10;
	Pos.y = static_cast<float>(y);

	// Create drop with radius ranging from 0.2 to 0.7 pixels
	Radius = (RandomGenerator::GetInstance().GenerateInt(2, 7) * 0.1f) * pDisplayData->ScaleFactor;

	// Initialize velocity and physics parameters
	const float scaleFactor = pDisplayData->ScaleFactor;
	Vel.x = WIND_MULTIPLIER * WindDirectionFactor * scaleFactor;
	Vel.y = TERMINAL_VELOCITY_Y * scaleFactor;

	// Initialize length of the rain drop trail
	DropTrailLength = RandomGenerator::GetInstance().GenerateInt(30, 100) * scaleFactor;
}

RainDrop::~RainDrop() noexcept = default;

bool RainDrop::DidTouchGround() const noexcept
{
	return TouchedGround;
}

bool RainDrop::IsReadyForErase() const noexcept
{
	return IsDead;
}

void RainDrop::UpdatePosition(const float deltaSeconds) noexcept
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
		for (const auto& splatter : Splatters)
		{
			splatter->UpdatePosition(deltaSeconds);
		}
		IsDead = (++CurrentFrameCountForSplatter) >= MAX_SPLUTTER_FRAME_COUNT_;
	}
}

void RainDrop::CreateSplatters() noexcept
{
	const float scaleFactor = pDisplayData->ScaleFactor;
	const float splatterVelocity = SPLATTER_STARTING_VELOCITY * scaleFactor;
	
	for (int i = 0; i < MAX_SPLATTER_PER_RAINDROP_; ++i)
	{
		const int angleBounce = RandomGenerator::GetInstance().GenerateInt(20, 70, 110, 160);
		const float angleBounceRadians = angleBounce * (PI / 180.0f);

		// Calculate velocity components using cached values
		const Vector2 velSplatter(
			splatterVelocity * std::cos(angleBounceRadians),
			-splatterVelocity * std::sin(angleBounceRadians));

		Splatters.emplace_back(std::make_unique<Splatter>(pDisplayData, Pos, velSplatter));
	}
}

void RainDrop::Draw(ID2D1DeviceContext* dc) const noexcept
{
	const Vector2 prevPoint = MathUtil::FindFirstPoint(DropTrailLength, Pos, Vel);

	if (ShouldDrawRainLine(prevPoint))
	{
		if (MathUtil::IsPointInRect(pDisplayData->SceneRect, Pos) && 
			MathUtil::IsPointInRect(pDisplayData->SceneRect, prevPoint))
		{
			dc->DrawLine(prevPoint.ToD2DPoint(), Pos.ToD2DPoint(), 
			             pDisplayData->DropColorBrush.Get(), Radius);
		}
		else if (MathUtil::IsPointInRect(pDisplayData->SceneRect, Pos) || 
		         MathUtil::IsPointInRect(pDisplayData->SceneRect, prevPoint))
		{
			D2D1_POINT_2F startPoint, endPoint;
			MathUtil::TrimLineSegment(pDisplayData->SceneRect, prevPoint.ToD2DPoint(), 
			                          Pos.ToD2DPoint(), startPoint, endPoint);
			dc->DrawLine(startPoint, endPoint, pDisplayData->DropColorBrush.Get(), Radius);
		}
	}

	if (!Splatters.empty())
	{
		for (const auto& splatter : Splatters)
		{
			splatter->Draw(dc, pDisplayData->PrebuiltSplatterOpacityBrushes[CurrentFrameCountForSplatter].Get());
		}
	}
}

bool RainDrop::ShouldDrawRainLine(const Vector2& prevPoint) const noexcept
{
	return !TouchedGround && (MathUtil::IsPointInRect(pDisplayData->SceneRect, Pos) || 
	                          MathUtil::IsPointInRect(pDisplayData->SceneRect, prevPoint));
}
