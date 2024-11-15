#include "SnowFlake.h"
#include "RandomGenerator.h"
#include "MathUtil.h"

#include "FastNoiseLite.h"


SnowFlake::SnowFlake(DisplayData* pDispData) :
	pDisplayData(pDispData)
{	
	Spawn();

}

void SnowFlake::Spawn()
{
	Pos.x = RandomGenerator::GetInstance().GenerateInt(-pDisplayData->Width / 2, (pDisplayData->Width * 3) / 2);
	Pos.y = RandomGenerator::GetInstance().GenerateInt(-pDisplayData->Height / 2, pDisplayData->Height);
	Vel.x = 0.0f;
	Vel.y = RandomGenerator::GetInstance().GenerateInt(5.0f, 20.0f);
}

void SnowFlake::ReSpawn()
{
	Pos.x = RandomGenerator::GetInstance().GenerateInt(-pDisplayData->Width * 0.5f, pDisplayData->Width * 1.5f);
	Pos.y = -5.0f;
	Vel.x = 0.0f;
	Vel.y = RandomGenerator::GetInstance().GenerateInt(5.0f, 20.0f);
}

void SnowFlake::UpdatePosition(const float deltaSeconds)
{
	const float t = static_cast<float>(clock()) * NOISE_TIMESCALE;
    const float noiseVal = pDisplayData->pNoiseGen->GetNoise(Pos.x , Pos.y , t );
	const float angle = noiseVal * TWO_PI + PI * 0.5f;

	Vel.x += (std::cos(angle) * NOISE_INTENSITY * deltaSeconds) * 2.0f;
	Vel.y += std::sin(angle) * NOISE_INTENSITY * deltaSeconds;

	Vel.y += GRAVITY * deltaSeconds;

	if (Vel.magSq() > MAX_SPEED * MAX_SPEED)
	{
		Vel.setMag(MAX_SPEED);
	}

	Pos.x += Vel.x * deltaSeconds;
	Pos.y += Vel.y * deltaSeconds;

	if (Pos.x < -pDisplayData->Width * 0.5f || 
		Pos.x >= pDisplayData->Width * 1.5f || 
		Pos.y < -pDisplayData->Height *	0.5f || 
		Pos.y >= pDisplayData->Height)
	{
		if (Pos.x >= 0 && Pos.x < pDisplayData->Width && Pos.y >= pDisplayData->Height)
		{
			const int x = Pos.x;
			pDisplayData->pScenePixels[x + (pDisplayData->Height - 1) * pDisplayData->Width] = SNOW_COLOR;
		}
		ReSpawn();
	}

	// If any of our neighboring pixels are filled, settle here
	const int x = Pos.x;
	const int y = Pos.y;

	if (x >= 0 && x < pDisplayData->Width && y >= 0 && y < pDisplayData->Height)
	{
		for (int xOff = -1; xOff <= 1; ++xOff)
		{
			for (int yOff = -1; yOff <= 1; ++yOff)
			{
				if (IsSceneryPixelSet(x + xOff, y + yOff))
				{
					if (pDisplayData->pScenePixels[x + y * pDisplayData->Width] == AIR_COLOR)
					{
						// Only settle if the pixel is empty
						pDisplayData->pScenePixels[x + y * pDisplayData->Width] = SNOW_COLOR;
						if (y < pDisplayData->MaxSnowHeight)
						{
							pDisplayData->MaxSnowHeight = y;
						}
					}
					ReSpawn();
					return;
				}
			}
		}
	}
}

void SnowFlake::Draw(ID2D1DeviceContext* dc) const
{
	if (MathUtil::IsPointInRect(pDisplayData->WindowRectNorm, Pos))
	{
		// Define the ellipse with center at (posX, posY) and radius 5px
		const D2D1_ELLIPSE ellipse = D2D1::Ellipse(
			D2D1::Point2F(Pos.x + pDisplayData->WindowRect.left, Pos.y + pDisplayData->WindowRect.top), 
			2.0f * pDisplayData->ScaleFactor, 
			2.0f * pDisplayData->ScaleFactor);
		// Draw the ellipse
		dc->FillEllipse(ellipse, pDisplayData->DropColorBrush.Get());
	}
}

void SnowFlake::DrawSettledSnow2(ID2D1DeviceContext* dc, const DisplayData* pDispData)
{
	for (int y = pDispData->Height - 1; y >= pDispData->MaxSnowHeight; --y)
	{
		for (int x = 0; x < pDispData->Width; ++x)
		{
			if (pDispData->pScenePixels[x + y * pDispData->Width] == SNOW_COLOR)
			{
				const int normX = x + pDispData->WindowRect.left;
				const int normY = y + pDispData->WindowRect.top;
				const float halfWidth = pDispData->ScaleFactor >= 1 ? pDispData->ScaleFactor : 1;

				D2D1_RECT_F rect = D2D1::RectF(
					normX - halfWidth,
					normY - halfWidth,
					normX + halfWidth,
					normY + halfWidth);
				dc->FillRectangle(rect, pDispData->DropColorBrush.Get());

				//// Define the ellipse with center at (posX, posY) and radius 5px
				//D2D1_ELLIPSE ellipse = D2D1::Ellipse(
				//	D2D1::Point2F(normX, y + normY), 2.0f, 2.0f);

				//// Draw the ellipse
				//dc->FillEllipse(ellipse, pDispData->DropColorBrush.Get());
			}
		}
	}
}

void SnowFlake::DrawSettledSnow(ID2D1DeviceContext* dc, const DisplayData* pDispData)
{
	for (int y = pDispData->Height - 1; y >= pDispData->MaxSnowHeight; --y)
	{
		int startX = -1; // Start of the run of SNOW_COLOR pixels

		for (int x = 0; x < pDispData->Width; ++x)
		{
			if (pDispData->pScenePixels[x + y * pDispData->Width] == SNOW_COLOR)
			{
				if (startX == -1) // New run starts
				{
					startX = x;
				}

				// If we reach the end of the row or the next pixel is not SNOW_COLOR
				if (x == pDispData->Width - 1 || pDispData->pScenePixels[(x + 1) + y * pDispData->Width] != SNOW_COLOR)
				{
					const int normXStart = startX + pDispData->WindowRect.left;
					const int normXEnd = x + pDispData->WindowRect.left;
					const int normY = y + pDispData->WindowRect.top;
					const float halfWidth = pDispData->ScaleFactor >= 1 ? pDispData->ScaleFactor : 1;

					D2D1_RECT_F rect = D2D1::RectF(
						normXStart - halfWidth,
						normY - halfWidth,
						normXEnd + halfWidth,
						normY + halfWidth
					);

					dc->FillRectangle(rect, pDispData->DropColorBrush.Get());

					// Reset startX for the next run
					startX = -1;
				}
			}
			else
			{
				startX = -1; // No more consecutive pixels in this row
			}
		}
	}
}

bool SnowFlake::CanSnowFlowInto(const int x, const int y, const DisplayData* pDispData)
{
	if (x < 0 || x >= pDispData->Width || y < 0 || y >= pDispData->Height) return false; // Out-of-bounds
	const bool pixel = pDispData->pScenePixels[x + y * pDispData->Width];
	return pixel == AIR_COLOR;
}

bool SnowFlake::IsSceneryPixelSet(const int x, const int y) const
{
	if (x < 0 || x >= pDisplayData->Width || y < 0 || y >= pDisplayData->Height) return false; // Out-of-bounds
	const bool pixel = pDisplayData->pScenePixels[x + y * pDisplayData->Width];
	return pixel == SNOW_COLOR;
}

void SnowFlake::SettleSnow(const DisplayData* pDispData)
{
	// Settled snow physics
	// Iterate from bottom-up, to avoid updating falling pixels multiple times per-frame, which would cause them to "teleport"
	for (int y = pDispData->Height - 1; y >= pDispData->MaxSnowHeight; --y)
	{
		for (int x = 0; x < pDispData->Width; ++x)
		{
			const bool pixel = pDispData->pScenePixels[x + y * pDispData->Width];
			if (pixel != SNOW_COLOR) continue;
			if (RandomGenerator::GetInstance().GenerateInt(0, 10) > SNOW_FLOW_RATE) continue;

			if (CanSnowFlowInto(x, y + 1, pDispData))
			{
				// Flow downwards
				pDispData->pScenePixels[x + (y + 1) * pDispData->Width] = SNOW_COLOR;
				pDispData->pScenePixels[x + y * pDispData->Width] = AIR_COLOR;
			}
			else
			{
				// Try to flow down and left/right
				// Randomly try either left or right first, so we're less biased
				const int firstDirection = RandomGenerator::GetInstance().GenerateInt(0, 100) < 50 ? -1 : 1;
				const int secondDirection = -firstDirection;

				if (CanSnowFlowInto(x + firstDirection, y + 1, pDispData) && CanSnowFlowInto(
					x + firstDirection, y, pDispData))
				{
					pDispData->pScenePixels[x + firstDirection + (y + 1) * pDispData->Width] = SNOW_COLOR;
					pDispData->pScenePixels[x + y * pDispData->Width] = AIR_COLOR;
				}
				else if (CanSnowFlowInto(x + secondDirection, y + 1, pDispData) && CanSnowFlowInto(
					x + secondDirection, y, pDispData))
				{
					pDispData->pScenePixels[x + secondDirection + (y + 1) * pDispData->Width] = SNOW_COLOR;
					pDispData->pScenePixels[x + y * pDispData->Width] = AIR_COLOR;
				}
			}
		}
	}
}
