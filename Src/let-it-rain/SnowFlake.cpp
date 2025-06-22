#include "SnowFlake.h"
#include "RandomGenerator.h"
#include "MathUtil.h"

#include "FastNoiseLite.h"
#include <ctime>

// Define the static member variable
float SnowFlake::s_snowAccumulationChance = 0.05f;

// Static variable to track frames for settling snow
static int s_snowSettleFrameCounter = 0;

SnowFlake::SnowFlake(DisplayData* pDispData) :
	pDisplayData(pDispData)
{	
	Spawn();
}

void SnowFlake::Spawn()
{
	auto& rng = RandomGenerator::GetInstance();
	
	// Position randomization
	Pos.x = rng.GenerateInt(-pDisplayData->Width / 2, (pDisplayData->Width * 3) / 2);
	Pos.y = rng.GenerateInt(-pDisplayData->Height / 2, pDisplayData->Height);
	
	// Velocity randomization - increased speed for more dynamic movement
	Vel.x = rng.GenerateInt(-15, 15); // Add initial horizontal velocity variation
	Vel.y = rng.GenerateInt(25.0f, 75.0f); // Increased speed range for more dynamic movement
	
	// Visual properties
	Size = 0.5f + (rng.GenerateInt(0, 80) / 100.0f); // 0.5 to 1.3 base size for more variation
	Rotation = rng.GenerateInt(0, 360) * (PI / 180.0f); // Random initial rotation
	RotationSpeed = (rng.GenerateInt(-60, 60) / 300.0f); // Increased rotation speed variation
	Opacity = 0.5f + (rng.GenerateInt(0, 50) / 100.0f); // 0.5 to 1.0 opacity for more variation
	
	// Wobble effect for more natural movement
	WobblePhase = rng.GenerateInt(0, 100) / 100.0f * TWO_PI;
	WobbleAmplitude = rng.GenerateInt(20, 200) / 100.0f * MAX_WOBBLE; // Increased wobble variation
	
	// Randomly choose a snowflake shape
	int shapeType = rng.GenerateInt(0, 100);
	if (shapeType < 40) {
		Shape = SnowflakeShape::Simple; // 40% simple shapes
	} else if (shapeType < 70) {
		Shape = SnowflakeShape::Crystal; // 30% crystals
	} else if (shapeType < 90) {
		Shape = SnowflakeShape::Hexagon; // 20% hexagons
	} else {
		Shape = SnowflakeShape::Star; // 10% stars
	}
}

void SnowFlake::ReSpawn()
{
	auto& rng = RandomGenerator::GetInstance();
	
	// Position randomization (above the screen)
	Pos.x = rng.GenerateInt(-pDisplayData->Width * 0.5f, pDisplayData->Width * 1.5f);
	Pos.y = -5.0f;
	
	// Velocity randomization
	Vel.x = rng.GenerateInt(-15, 15); // Increased horizontal velocity variation
	Vel.y = rng.GenerateInt(25.0f, 75.0f); // Increased speed range for more dynamic movement
	
	// Visual properties
	Size = 0.5f + (rng.GenerateInt(0, 80) / 100.0f); // 0.5 to 1.3 base size for more variation
	Rotation = rng.GenerateInt(0, 360) * (PI / 180.0f);
	RotationSpeed = (rng.GenerateInt(-60, 60) / 300.0f); // Increased rotation speed variation
	Opacity = 0.5f + (rng.GenerateInt(0, 50) / 100.0f); // 0.5 to 1.0 opacity for more variation
	
	// Wobble effect
	WobblePhase = rng.GenerateInt(0, 100) / 100.0f * TWO_PI;
	WobbleAmplitude = rng.GenerateInt(20, 200) / 100.0f * MAX_WOBBLE; // Increased wobble variation
	
	// Randomly choose a snowflake shape (same distribution as in Spawn)
	int shapeType = rng.GenerateInt(0, 100);
	if (shapeType < 40) {
		Shape = SnowflakeShape::Simple;
	} else if (shapeType < 70) {
		Shape = SnowflakeShape::Crystal;
	} else if (shapeType < 90) {
		Shape = SnowflakeShape::Hexagon;
	} else {
		Shape = SnowflakeShape::Star;
	}
}

void SnowFlake::UpdatePosition(const float deltaSeconds)
{
	// Update rotation
	Rotation += RotationSpeed * deltaSeconds;
	
	// Update wobble phase
	WobblePhase += deltaSeconds;
	if (WobblePhase > TWO_PI) {
		WobblePhase -= TWO_PI;
	}
	
	// Apply noise-based movement
	const float t = static_cast<float>(clock()) * NOISE_TIMESCALE;
    const float noiseVal = pDisplayData->pNoiseGen->GetNoise(Pos.x, Pos.y, t);
	const float angle = noiseVal * TWO_PI + PI * 0.5f;

	// Add wobble effect to horizontal movement
	float wobbleEffect = sin(WobblePhase) * WobbleAmplitude;
	
	Vel.x += (std::cos(angle) * NOISE_INTENSITY * deltaSeconds) * 2.0f + wobbleEffect * deltaSeconds;
	Vel.y += std::sin(angle) * NOISE_INTENSITY * deltaSeconds;

	Vel.y += GRAVITY * deltaSeconds;

	// Dampen horizontal velocity slightly for more realistic movement
	Vel.x *= 0.99f;

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
	if (MathUtil::IsPointInRect(pDisplayData->SceneRectNorm, Pos))
	{
		// Calculate the drawing position
		D2D1_POINT_2F center = D2D1::Point2F(
			Pos.x + pDisplayData->SceneRect.left,
			Pos.y + pDisplayData->SceneRect.top
		);
		
		// Create a temporary brush with adjusted opacity
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> tempBrush;
		D2D1_COLOR_F baseColor = pDisplayData->DropColorBrush->GetColor();
		baseColor.a = Opacity; // Apply snowflake's opacity
		dc->CreateSolidColorBrush(baseColor, tempBrush.GetAddressOf());
		
		// Scale the size based on the display scale factor
		float drawSize = Size * pDisplayData->ScaleFactor;
		
		// Draw different snowflake shapes based on the shape type
		switch (Shape)
		{
		case SnowflakeShape::Simple:
			DrawSimpleSnowflake(dc, center, drawSize, Rotation);
			break;
		case SnowflakeShape::Crystal:
			DrawCrystalSnowflake(dc, center, drawSize, Rotation);
			break;
		case SnowflakeShape::Hexagon:
			DrawHexagonSnowflake(dc, center, drawSize, Rotation);
			break;
		case SnowflakeShape::Star:
			DrawStarSnowflake(dc, center, drawSize, Rotation);
			break;
		}
	}
}

void SnowFlake::DrawSimpleSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const
{
	// For simple snowflakes, just draw an ellipse with slight variations
	const float radiusX = 1.0f * size;
	const float radiusY = 0.7f * size;
	
	// Create a rotation matrix
	D2D1::Matrix3x2F rotMatrix = D2D1::Matrix3x2F::Rotation(
		rotation * 180.0f / PI, // Convert to degrees
		center
	);
	
	// Save the current transform
	D2D1::Matrix3x2F originalTransform;
	dc->GetTransform(&originalTransform);
	
	// Apply rotation
	dc->SetTransform(rotMatrix * originalTransform);
	
	// Draw the ellipse
	D2D1_ELLIPSE ellipse = D2D1::Ellipse(center, radiusX, radiusY);
	dc->FillEllipse(ellipse, pDisplayData->DropColorBrush.Get());
	
	// Restore original transform
	dc->SetTransform(originalTransform);
}

void SnowFlake::DrawCrystalSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const
{
	// Save the current transform
	D2D1::Matrix3x2F originalTransform;
	dc->GetTransform(&originalTransform);
	
	// Apply rotation
	D2D1::Matrix3x2F rotMatrix = D2D1::Matrix3x2F::Rotation(
		rotation * 180.0f / PI,
		center
	);
	dc->SetTransform(rotMatrix * originalTransform);
	
	// Draw a small center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.5f, size * 0.5f);
	dc->FillEllipse(centerCircle, pDisplayData->DropColorBrush.Get());
	
	// Draw 6 arms for the crystal (60 degrees apart)
	const int numArms = 6;
	const float baseLength = size * 2.0f;
	
	for (int i = 0; i < numArms; i++)
	{
		float angle = (i * TWO_PI) / numArms;
		float endX = center.x + cos(angle) * baseLength;
		float endY = center.y + sin(angle) * baseLength;
		
		// Create a line for each arm
		D2D1_POINT_2F endPoint = D2D1::Point2F(endX, endY);
		
		// Draw the main arm
		dc->DrawLine(center, endPoint, pDisplayData->DropColorBrush.Get(), size * 0.2f);
		
		// Draw small branches (2 per arm)
		float branchLength = baseLength * 0.4f;
		float branchAngleOffset = 30.0f * (PI / 180.0f); // 30 degrees
		
		float midX = center.x + cos(angle) * baseLength * 0.6f;
		float midY = center.y + sin(angle) * baseLength * 0.6f;
		D2D1_POINT_2F midPoint = D2D1::Point2F(midX, midY);
		
		// First branch
		float branch1Angle = angle + branchAngleOffset;
		float branch1EndX = midX + cos(branch1Angle) * branchLength;
		float branch1EndY = midY + sin(branch1Angle) * branchLength;
		D2D1_POINT_2F branch1End = D2D1::Point2F(branch1EndX, branch1EndY);
		dc->DrawLine(midPoint, branch1End, pDisplayData->DropColorBrush.Get(), size * 0.15f);
		
		// Second branch
		float branch2Angle = angle - branchAngleOffset;
		float branch2EndX = midX + cos(branch2Angle) * branchLength;
		float branch2EndY = midY + sin(branch2Angle) * branchLength;
		D2D1_POINT_2F branch2End = D2D1::Point2F(branch2EndX, branch2EndY);
		dc->DrawLine(midPoint, branch2End, pDisplayData->DropColorBrush.Get(), size * 0.15f);
	}
	
	// Restore original transform
	dc->SetTransform(originalTransform);
}

void SnowFlake::DrawHexagonSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const
{
	// Save the current transform
	D2D1::Matrix3x2F originalTransform;
	dc->GetTransform(&originalTransform);
	
	// Apply rotation
	D2D1::Matrix3x2F rotMatrix = D2D1::Matrix3x2F::Rotation(
		rotation * 180.0f / PI,
		center
	);
	dc->SetTransform(rotMatrix * originalTransform);
	
	// Draw a hexagon shape using lines
	const int sides = 6;
	const float radius = size * 2.0f;
	
	// Draw the hexagon outline
	D2D1_POINT_2F* points = new D2D1_POINT_2F[sides + 1];
	
	for (int i = 0; i <= sides; ++i) {
		float angle = i * TWO_PI / sides;
		points[i] = D2D1::Point2F(
			center.x + radius * cos(angle),
			center.y + radius * sin(angle)
		);
	}
	
	// Draw the hexagon outline
	for (int i = 0; i < sides; ++i) {
		dc->DrawLine(points[i], points[i + 1], pDisplayData->DropColorBrush.Get(), size * 0.2f);
	}
	
	// Draw inner details (spokes)
	for (int i = 0; i < sides; ++i) {
		// Draw line from center to each vertex
		dc->DrawLine(
			center,
			points[i],
			pDisplayData->DropColorBrush.Get(),
			size * 0.15f
		);
	}
	
	delete[] points;
	
	// Draw center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.4f, size * 0.4f);
	dc->FillEllipse(centerCircle, pDisplayData->DropColorBrush.Get());
	
	// Restore original transform
	dc->SetTransform(originalTransform);
}

void SnowFlake::DrawStarSnowflake(ID2D1DeviceContext* dc, D2D1_POINT_2F center, float size, float rotation) const
{
	// Save the current transform
	D2D1::Matrix3x2F originalTransform;
	dc->GetTransform(&originalTransform);
	
	// Apply rotation
	D2D1::Matrix3x2F rotMatrix = D2D1::Matrix3x2F::Rotation(
		rotation * 180.0f / PI,
		center
	);
	dc->SetTransform(rotMatrix * originalTransform);
	
	// Draw a small center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.4f, size * 0.4f);
	dc->FillEllipse(centerCircle, pDisplayData->DropColorBrush.Get());
	
	// Draw a star pattern with 12 spikes
	const int numSpikes = 12;
	const float outerRadius = size * 2.5f;
	const float innerRadius = size * 1.0f;
	
	for (int i = 0; i < numSpikes; i++)
	{
		float angle = (i * TWO_PI) / numSpikes;
		
		// Main spike
		float endX = center.x + cos(angle) * outerRadius;
		float endY = center.y + sin(angle) * outerRadius;
		D2D1_POINT_2F endPoint = D2D1::Point2F(endX, endY);
		
		// Draw the spike
		dc->DrawLine(center, endPoint, pDisplayData->DropColorBrush.Get(), size * 0.15f);
		
		// Draw small intersecting lines between main spikes
		if (i % 2 == 0) {
			float crossAngle = angle + (TWO_PI / numSpikes / 2);
			float crossX = center.x + cos(crossAngle) * innerRadius;
			float crossY = center.y + sin(crossAngle) * innerRadius;
			D2D1_POINT_2F crossPoint = D2D1::Point2F(crossX, crossY);
			
			dc->DrawLine(center, crossPoint, pDisplayData->DropColorBrush.Get(), size * 0.1f);
		}
	}
	
	// Restore original transform
	dc->SetTransform(originalTransform);
}

void SnowFlake::DrawSettledSnow2(ID2D1DeviceContext* dc, const DisplayData* pDispData)
{
	for (int y = pDispData->Height - 1; y >= pDispData->MaxSnowHeight; --y)
	{
		for (int x = 0; x < pDispData->Width; ++x)
		{
			if (pDispData->pScenePixels[x + y * pDispData->Width] == SNOW_COLOR)
			{
				const int normX = x + pDispData->SceneRect.left;
				const int normY = y + pDispData->SceneRect.top;
				const float halfWidth = pDispData->ScaleFactor >= 1 ? pDispData->ScaleFactor : 1;

				D2D1_RECT_F rect = D2D1::RectF(
					normX - halfWidth,
					normY - halfWidth,
					normX + halfWidth,
					normY + halfWidth);
				dc->FillRectangle(rect, pDispData->DropColorBrush.Get());

				// Define the ellipse with center at (posX, posY) and radius 5px
				D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(normX, y + normY), 5.0f, 5.0f);

				// Draw the ellipse
				dc->FillEllipse(ellipse, pDispData->DropColorBrush.Get());
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
					const int normXStart = startX + pDispData->SceneRect.left;
					const int normXEnd = x + pDispData->SceneRect.left;
					const int normY = y + pDispData->SceneRect.top;
					const float halfWidth = pDispData->ScaleFactor >= 1 ? pDispData->ScaleFactor : 1;

					// rect settle snow
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
	// Frame skipping for slower snow settling
	// Only process snow settling every other frame
	s_snowSettleFrameCounter++;
	if (s_snowSettleFrameCounter % 2 != 0) {
		return; // Skip this frame
	}

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
					// Add a small chance to create additional snow - snow multiplication effect
				}
				else if (RandomGenerator::GetInstance().GenerateInt(0, 100) < s_snowAccumulationChance * 100) // Use the adjustable value
				{
					// Try to add snow to adjacent spots - this creates a small snow multiplication effect
					// which helps build up snow accumulation in certain areas
					for (int nx = -1; nx <= 1; nx++)
					{
						for (int ny = -1; ny <= 1; ny++)
						{
							if (nx == 0 && ny == 0) continue; // Skip self position
							if (CanSnowFlowInto(x + nx, y + ny, pDispData))
							{
								// Add a new snow pixel to an adjacent empty space
								pDispData->pScenePixels[(x + nx) + (y + ny) * pDispData->Width] = SNOW_COLOR;
								// Only add one extra snow pixel per iteration to prevent excessive growth
								nx = 2; ny = 2; // Break out of both loops by setting indices beyond their bounds
							}
						}
					}
				}
			}
		}
	}
}
