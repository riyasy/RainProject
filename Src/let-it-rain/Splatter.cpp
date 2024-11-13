#include "Splatter.h"
#include "MathUtil.h"
#include "RandomGenerator.h"

#include <d2d1.h>

Splatter::Splatter(DisplayData* pDispData, const Vector2 pos, const Vector2 vel) :
	pDisplayData(pDispData), Pos(pos), Vel(vel)
{
	// Create splatters with radius ranging from 1.0 to 2.0 pixels
	Radius = (RandomGenerator::GetInstance().GenerateInt(15, 25) / 10.0f) * pDisplayData->ScaleFactor;
	Pos.y = pos.y - Radius; // Slight adjustment
}

Splatter::~Splatter() = default;

void Splatter::UpdatePosition(float deltaSeconds)
{
	// Update the position of the raindrop
	Pos.x += Vel.x * deltaSeconds;
	Pos.y += Vel.y * deltaSeconds;

	Vel.y += GRAVITY; // Gravity
	Vel.x *= AIR_RESISTANCE; // Air Resistance

	// Check for bouncing against sides
	if (Pos.x + Radius > pDisplayData->WindowRect.right || Pos.x - Radius <
		pDisplayData->WindowRect.left)
	{
		Vel.x = -Vel.x;
	}
	// Check for bouncing against bottom border
	if (Pos.y + Radius > pDisplayData->WindowRect.bottom)
	{
		Pos.y = pDisplayData->WindowRect.bottom - Radius; // Keep the ellipse within bounds
		Vel.y = -Vel.y * BOUNCE_DAMPING; // Bounce with damping
		SplatterBounceCount++;
	}
	// Check for bouncing against top
	if (Pos.y - Radius < pDisplayData->WindowRect.top)
	{
		Pos.y = Radius; // Keep the ellipse within bounds
		Vel.y = -Vel.y; // Reverse the direction if it hits the top edge
	}
}

void Splatter::Draw(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const
{
	if (MathUtil::IsPointInRect(pDisplayData->WindowRect, Pos) &&
		SplatterBounceCount < MAX_SPLATTER_BOUNCE_COUNT_)
	{
		// Define the ellipse with center at (posX, posY) and radius 5px
		D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(Pos.x, Pos.y), Radius, Radius);
		dc->FillEllipse(ellipse, pBrush);
	}
}
