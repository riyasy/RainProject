#include "SnowFlake.h"
#include "RandomGenerator.h"

#define DB_PERLIN_IMPL
#include "db_perlin.hpp"

SnowFlake::SnowFlake(DisplayData* pDispData) :
	pDisplayData(pDispData)
{
	Spawn();
}

void SnowFlake::Spawn()
{
	pos.x = RandomGenerator::GetInstance().GenerateInt(-pDisplayData->Width * 0.5f, pDisplayData->Width * 1.5f);
	pos.y = RandomGenerator::GetInstance().GenerateInt(-pDisplayData->Height * 0.5f, pDisplayData->Height);
	vel.x = 0.0f;
	vel.y = RandomGenerator::GetInstance().GenerateInt(5.0f, 20.0f);
}

void SnowFlake::ReSpawn()
{
	pos.x = RandomGenerator::GetInstance().GenerateInt(-pDisplayData->Width * 0.5f, pDisplayData->Width * 1.5f);
	pos.y = -5.0f;
	vel.x = 0.0f;
	vel.y = RandomGenerator::GetInstance().GenerateInt(5.0f, 20.0f);
}

void SnowFlake::UpdatePosition(float deltaSeconds)
{
	float t = static_cast<float>(clock()) * NOISE_TIMESCALE;
	float noiseVal = db::perlin(pos.x * NOISE_SCALE, pos.y * NOISE_SCALE, t * NOISE_TIMESCALE);
	float angle = noiseVal * TWO_PI + PI * 0.5f;

	vel.x += (std::cos(angle) * NOISE_INTENSITY * deltaSeconds) * 2.0f;
	vel.y += std::sin(angle) * NOISE_INTENSITY * deltaSeconds;

	vel.y += GRAVITY * deltaSeconds;

	if (vel.magSq() > MAX_SPEED * MAX_SPEED)
	{
		vel.setMag(MAX_SPEED);
	}

	pos.x += vel.x * deltaSeconds;
	pos.y += vel.y * deltaSeconds;

	if (pos.x < -pDisplayData->Width * 0.5f || pos.x >= pDisplayData->Width * 1.5f || pos.y < -pDisplayData->Height *
		0.5f || pos.y >=
		pDisplayData->Height)
	{
		if (pos.x >= 0 && pos.x < pDisplayData->Width && pos.y >= pDisplayData->Height)
		{
			int x = pos.x;
			pDisplayData->scene[x + (pDisplayData->Height - 1) * pDisplayData->Width] = SNOW_COLOR;
		}
		ReSpawn();
	}

	// If any of our neighboring pixels are filled, settle here
	int x = pos.x;
	int y = pos.y;

	if (x >= 0 && x < pDisplayData->Width && y >= 0 && y < pDisplayData->Height)
	{
		for (int xOff = -1; xOff <= 1; ++xOff)
		{
			for (int yOff = -1; yOff <= 1; ++yOff)
			{
				if (IsSceneryPixelSet(x + xOff, y + yOff))
				{
					if (pDisplayData->scene[x + y * pDisplayData->Width] == AIR_COLOR)
					{
						// Only settle if the pixel is empty
						pDisplayData->scene[x + y * pDisplayData->Width] = SNOW_COLOR;
						if (y < pDisplayData->maxSnowHeight)
						{
							pDisplayData->maxSnowHeight = y;
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
	// Define the ellipse with center at (posX, posY) and radius 5px
	D2D1_ELLIPSE ellipse = D2D1::Ellipse(
		D2D1::Point2F(pos.x + pDisplayData->WindowRect.left, pos.y + pDisplayData->WindowRect.top), 2.0f, 2.0f);

	// Draw the ellipse
	dc->FillEllipse(ellipse, pDisplayData->DropColorBrush.Get());
}

void SnowFlake::DrawSettledSnow(ID2D1DeviceContext* dc, const DisplayData* pDispData)
{
	// Iterate through the 1D scene array with a single loop
	// TODO Lower snow heaps flickering
	for (int y = pDispData->Height - 1; y >= pDispData->maxSnowHeight; --y)
	{
		for (int x = 0; x < pDispData->Width; ++x)
		{
			if (pDispData->scene[x + y * pDispData->Width] == SNOW_COLOR)
			{
				// Define the ellipse with center at (posX, posY) and radius 5px
				D2D1_ELLIPSE ellipse = D2D1::Ellipse(
					D2D1::Point2F(x + pDispData->WindowRect.left, y + pDispData->WindowRect.top), 2.0f, 2.0f);

				// Draw the ellipse
				dc->FillEllipse(ellipse, pDispData->DropColorBrush.Get());
			}
		}
	}	
}

bool SnowFlake::CanSnowFlowInto(int x, int y, const DisplayData* pDispData)
{
	if (x < 0 || x >= pDispData->Width || y < 0 || y >= pDispData->Height) return false; // Out-of-bounds
	bool pixel = pDispData->scene[x + y * pDispData->Width];
	return pixel == AIR_COLOR;
}

bool SnowFlake::IsSceneryPixelSet(int x, int y) const
{
	if (x < 0 || x >= pDisplayData->Width || y < 0 || y >= pDisplayData->Height) return false; // Out-of-bounds
	bool pixel = pDisplayData->scene[x + y * pDisplayData->Width];
	return pixel == SNOW_COLOR;
}

void SnowFlake::SettleSnow(const DisplayData* pDispData)
{
	// Settled snow physics
	// Iterate from bottom-up, to avoid updating falling pixels multiple times per-frame, which would cause them to "teleport"
	for (int y = pDispData->Height - 1; y >= pDispData->maxSnowHeight; --y)
	{
		for (int x = 0; x < pDispData->Width; ++x)
		{
			bool pixel = pDispData->scene[x + y * pDispData->Width];
			if (pixel != SNOW_COLOR) continue;
			if (RandomGenerator::GetInstance().GenerateInt(0, 10) > SNOW_FLOW_RATE) continue;

			if (CanSnowFlowInto(x, y + 1, pDispData))
			{
				// Flow downwards
				pDispData->scene[x + (y + 1) * pDispData->Width] = SNOW_COLOR;
				pDispData->scene[x + y * pDispData->Width] = AIR_COLOR;
			}
			else
			{
				// Try to flow down and left/right
				// Randomly try either left or right first, so we're less biased
				int firstDirection = RandomGenerator::GetInstance().GenerateInt(0, 100) < 50 ? -1 : 1;
				int secondDirection = -firstDirection;

				if (CanSnowFlowInto(x + firstDirection, y + 1, pDispData) && CanSnowFlowInto(
					x + firstDirection, y, pDispData))
				{
					pDispData->scene[x + firstDirection + (y + 1) * pDispData->Width] = SNOW_COLOR;
					pDispData->scene[x + y * pDispData->Width] = AIR_COLOR;
				}
				else if (CanSnowFlowInto(x + secondDirection, y + 1, pDispData) && CanSnowFlowInto(
					x + secondDirection, y, pDispData))
				{
					pDispData->scene[x + secondDirection + (y + 1) * pDispData->Width] = SNOW_COLOR;
					pDispData->scene[x + y * pDispData->Width] = AIR_COLOR;
				}
			}
		}
	}
}
