#include "SnowFlake.h"
#include "RandomGenerator.h"
#include "MathUtil.h"
#include "FastNoiseLite.h"

SnowFlake::SnowFlake(DisplayData * pDispData) :
	pDisplayData(pDispData)
{
	Spawn();
}

// Move constructor
SnowFlake::SnowFlake(SnowFlake&& other) noexcept
{
	Pos = other.Pos;
	Vel = other.Vel;
	Size = other.Size;
	Rotation = other.Rotation;
	RotationSpeed = other.RotationSpeed;
	Shape = other.Shape;
	pDisplayData = other.pDisplayData;

	// leave other in safe state
	other.pDisplayData = nullptr;
}

// Move assignment
SnowFlake& SnowFlake::operator=(SnowFlake&& other) noexcept
{
	if (this != &other)
	{
		Pos = other.Pos;
		Vel = other.Vel;
		Size = other.Size;
		Rotation = other.Rotation;
		RotationSpeed = other.RotationSpeed;
		Shape = other.Shape;
		pDisplayData = other.pDisplayData;

		other.pDisplayData = nullptr;
	}
	return *this;
}

void SnowFlake::Spawn()
{
	Pos.x = RandomGenerator::GetInstance().GenerateFloat(-pDisplayData->Width / 2.0f, (pDisplayData->Width * 3) / 2.0f);
	Pos.y = RandomGenerator::GetInstance().GenerateFloat(-pDisplayData->Height / 2.0f, pDisplayData->Height / 1.0f);
	Vel.x = 0.0f;
	Vel.y = RandomGenerator::GetInstance().GenerateFloat(5.0f, 10.0f);

	Size = 0.5f + (RandomGenerator::GetInstance().GenerateFloat(0.0f, 0.8f)); // 0.5 to 1.3 base size for more variation
	Rotation = RandomGenerator::GetInstance().GenerateFloat(0.0f, TWO_PI); // Random initial rotation (in radians directly)
	RotationSpeed = RandomGenerator::GetInstance().GenerateFloat(-0.2f, 0.2f); // Reduced rotation speed for smoother spinning
	// Randomly choose a snowflake shape
	int shapeType = RandomGenerator::GetInstance().GenerateInt(0, 100);
	if (shapeType < 40) {
		Shape = SnowflakeShape::Simple; // 40% simple shapes
	}
	else if (shapeType < 70) {
		Shape = SnowflakeShape::Crystal; // 30% crystals
	}
	else if (shapeType < 90) {
		Shape = SnowflakeShape::Hexagon; // 20% hexagons
	}
	else {
		Shape = SnowflakeShape::Star; // 10% stars
	}
}

void SnowFlake::ReSpawn()
{
	Pos.x = RandomGenerator::GetInstance().GenerateFloat(-pDisplayData->Width * 0.5f, pDisplayData->Width * 1.5f);
	Pos.y = -5.0f;
	Vel.x = 0.0f;
	Vel.y = RandomGenerator::GetInstance().GenerateFloat(5.0f, 10.0f);

	// Visual properties
	Size = 0.5f + RandomGenerator::GetInstance().GenerateFloat(0.0f, 0.8f); // 0.5 to 1.3 base size
	Rotation = RandomGenerator::GetInstance().GenerateFloat(0.0f, TWO_PI); // Random initial rotation (in radians)
	RotationSpeed = RandomGenerator::GetInstance().GenerateFloat(-0.2f, 0.2f); // Reduced rotation speed
	// Randomly choose a snowflake shape (same distribution as in Spawn)
	int shapeType = RandomGenerator::GetInstance().GenerateInt(0, 100);
	if (shapeType < 40) {
		Shape = SnowflakeShape::Simple;
	}
	else if (shapeType < 70) {
		Shape = SnowflakeShape::Crystal;
	}
	else if (shapeType < 90) {
		Shape = SnowflakeShape::Hexagon;
	}
	else {
		Shape = SnowflakeShape::Star;
	}
}

void SnowFlake::UpdatePosition(const float deltaSeconds, double clockTime)
{
	//const float t = static_cast<float>(clock()) * NOISE_TIMESCALE;
	const float t = static_cast<float>(clockTime) * NOISE_TIMESCALE * 1000.0f;
	const float noiseVal = pDisplayData->pNoiseGen->GetNoise(Pos.x, Pos.y, t);
	const float angle = noiseVal * TWO_PI + PI * 0.5f;

	Vel.x += (std::cos(angle) * NOISE_INTENSITY * deltaSeconds) * 2.0f;
	Vel.y += std::sin(angle) * NOISE_INTENSITY * deltaSeconds;

	Vel.y += GRAVITY * deltaSeconds;

	if (Vel.magSq() > MAX_SPEED * MAX_SPEED)
	{
		Vel.setMag(MAX_SPEED);
	}

	// Update rotation
	Rotation += RotationSpeed * deltaSeconds;

	Pos.x += Vel.x * deltaSeconds;
	Pos.y += Vel.y * deltaSeconds;

	if (Pos.x < -pDisplayData->Width * 0.5f ||
		Pos.x >= pDisplayData->Width * 1.5f ||
		Pos.y < -pDisplayData->Height * 0.5f ||
		Pos.y >= pDisplayData->Height)
	{
		if (Pos.x >= 0 && Pos.x < pDisplayData->Width && Pos.y >= pDisplayData->Height)
		{
			const int x = Pos.x;
			pDisplayData->ScenePixels[x + (pDisplayData->Height - 1) * pDisplayData->Width] = 1; // SNOW_COLOR
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
					if (pDisplayData->ScenePixels[x + y * pDisplayData->Width] == 0)
					{
						// Only settle if the pixel is empty
						pDisplayData->ScenePixels[x + y * pDisplayData->Width] = 1; // SNOW_COLOR
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

void SnowFlake::GenerateSprite(ID2D1DeviceContext* dc, int shapeIndex) const
{
	// Create a compatible render target (bitmap)
	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> bmpRT;

	// Create a square sprite canvas
	D2D1_SIZE_F size = D2D1::SizeF(SPRITE_SIZE, SPRITE_SIZE);

	HRESULT hr = dc->CreateCompatibleRenderTarget(size, &bmpRT);
	if (FAILED(hr)) return;

	bmpRT->BeginDraw();
	bmpRT->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent

	// Draw center of the sprite
	D2D1_POINT_2F center = D2D1::Point2F(SPRITE_SIZE / 2.0f, SPRITE_SIZE / 2.0f);

	// Draw the shape upright (rotation 0)
	switch (static_cast<SnowflakeShape>(shapeIndex))
	{
	case SnowflakeShape::Simple:
		DrawSimpleSnowflake(bmpRT.Get(), center, SPRITE_BASE_DRAW_SIZE);
		break;
	case SnowflakeShape::Crystal:
		DrawCrystalSnowflake(bmpRT.Get(), center, SPRITE_BASE_DRAW_SIZE);
		break;
	case SnowflakeShape::Hexagon:
		DrawHexagonSnowflake(bmpRT.Get(), center, SPRITE_BASE_DRAW_SIZE);
		break;
	case SnowflakeShape::Star:
		DrawStarSnowflake(bmpRT.Get(), center, SPRITE_BASE_DRAW_SIZE);
		break;
	}

	bmpRT->EndDraw();

	// Store result in cache
	Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
	hr = bmpRT->GetBitmap(&bitmap);
	if (SUCCEEDED(hr))
	{
		pDisplayData->SpriteCache[shapeIndex] = bitmap;
	}
}

void SnowFlake::Draw(ID2D1DeviceContext* dc) const
{
	if (MathUtil::IsPointInRect(pDisplayData->SceneRectNorm, Pos))
	{
		int shapeIndex = static_cast<int>(Shape);

		// Lazy Load: If sprite doesn't exist, create it
		if (pDisplayData->SpriteCache[shapeIndex] == nullptr)
		{
			GenerateSprite(dc, shapeIndex);
		}

		ID2D1Bitmap* pBitmap = pDisplayData->SpriteCache[shapeIndex].Get();
		if (pBitmap)
		{
			D2D1_POINT_2F center = D2D1::Point2F(
				Pos.x + pDisplayData->SceneRect.left,
				Pos.y + pDisplayData->SceneRect.top
			);

			float drawWidth = SPRITE_SIZE * Size * pDisplayData->ScaleFactor;
			float drawHeight = SPRITE_SIZE * Size * pDisplayData->ScaleFactor;

			D2D1_RECT_F destRect = D2D1::RectF(
				center.x - drawWidth / 2.0f,
				center.y - drawHeight / 2.0f,
				center.x + drawWidth / 2.0f,
				center.y + drawHeight / 2.0f
			);

			// Apply Rotation
			D2D1::Matrix3x2F rotMatrix = D2D1::Matrix3x2F::Rotation(Rotation * 180.0f / PI,	center);
			D2D1::Matrix3x2F originalTransform;
			dc->GetTransform(&originalTransform);
			dc->SetTransform(rotMatrix * originalTransform);

			// Draw the cached bitmap
			dc->DrawBitmap(pBitmap, destRect);

			dc->SetTransform(originalTransform);
		}
	}
}

// NOTE: All helpers now take ID2D1RenderTarget* to support BitmapRenderTarget

void SnowFlake::DrawSimpleSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size) const
{
	// For simple snowflakes, just draw an ellipse with slight variations
	const float radiusX = 1.0f * size;
	const float radiusY = 0.7f * size;

	// Draw the ellipse
	D2D1_ELLIPSE ellipse = D2D1::Ellipse(center, radiusX, radiusY);
	rt->FillEllipse(ellipse, pDisplayData->DropColorBrush.Get());
}

void SnowFlake::DrawCrystalSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size) const
{
	// Draw a small center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.5f, size * 0.5f);
	rt->FillEllipse(centerCircle, pDisplayData->DropColorBrush.Get());

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
		rt->DrawLine(center, endPoint, pDisplayData->DropColorBrush.Get(), size * 0.2f);

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
		rt->DrawLine(midPoint, branch1End, pDisplayData->DropColorBrush.Get(), size * 0.15f);

		// Second branch
		float branch2Angle = angle - branchAngleOffset;
		float branch2EndX = midX + cos(branch2Angle) * branchLength;
		float branch2EndY = midY + sin(branch2Angle) * branchLength;
		D2D1_POINT_2F branch2End = D2D1::Point2F(branch2EndX, branch2EndY);
		rt->DrawLine(midPoint, branch2End, pDisplayData->DropColorBrush.Get(), size * 0.15f);
	}
}

void SnowFlake::DrawHexagonSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size) const
{
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
		rt->DrawLine(points[i], points[i + 1], pDisplayData->DropColorBrush.Get(), size * 0.2f);
	}

	// Draw inner details (spokes)
	for (int i = 0; i < sides; ++i) {
		// Draw line from center to each vertex
		rt->DrawLine(
			center,
			points[i],
			pDisplayData->DropColorBrush.Get(),
			size * 0.15f
		);
	}

	delete[] points;

	// Draw center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.4f, size * 0.4f);
	rt->FillEllipse(centerCircle, pDisplayData->DropColorBrush.Get());
}

void SnowFlake::DrawStarSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size) const
{
	// Draw a small center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.4f, size * 0.4f);
	rt->FillEllipse(centerCircle, pDisplayData->DropColorBrush.Get());

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
		rt->DrawLine(center, endPoint, pDisplayData->DropColorBrush.Get(), size * 0.15f);

		// Draw small intersecting lines between main spikes
		if (i % 2 == 0) {
			float crossAngle = angle + (TWO_PI / numSpikes / 2);
			float crossX = center.x + cos(crossAngle) * innerRadius;
			float crossY = center.y + sin(crossAngle) * innerRadius;
			D2D1_POINT_2F crossPoint = D2D1::Point2F(crossX, crossY);

			rt->DrawLine(center, crossPoint, pDisplayData->DropColorBrush.Get(), size * 0.1f);
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
			if (pDispData->ScenePixels[x + y * pDispData->Width] == 1)
			{
				if (startX == -1) // New run starts
				{
					startX = x;
				}

				// If we reach the end of the row or the next pixel is not SNOW_COLOR
				if (x == pDispData->Width - 1 || pDispData->ScenePixels[(x + 1) + y * pDispData->Width] != 1)
				{
					const int normXStart = startX + pDispData->SceneRect.left;
					const int normXEnd = x + pDispData->SceneRect.left;
					const int normY = y + pDispData->SceneRect.top;
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
	const uint8_t pixel = pDispData->ScenePixels[x + y * pDispData->Width];
	return pixel == 0; // AIR_COLOR
}

bool SnowFlake::IsSceneryPixelSet(const int x, const int y) const
{
	if (x < 0 || x >= pDisplayData->Width || y < 0 || y >= pDisplayData->Height) return false; // Out-of-bounds
	const uint8_t pixel = pDisplayData->ScenePixels[x + y * pDisplayData->Width];
	return pixel == 1; // SNOW_COLOR
}

void SnowFlake::SettleSnow(DisplayData* pDispData)
{
	// Settled snow physics
	// Iterate from bottom-up, to avoid updating falling pixels multiple times per-frame, which would cause them to "teleport"
	for (int y = pDispData->Height - 1; y >= pDispData->MaxSnowHeight; --y)
	{
		for (int x = 0; x < pDispData->Width; ++x)
		{
			const uint8_t pixel = pDispData->ScenePixels[x + y * pDispData->Width];
			if (pixel != 1) continue;
			if (RandomGenerator::GetInstance().GenerateInt(0, 10) > SNOW_FLOW_RATE) continue;

			if (CanSnowFlowInto(x, y + 1, pDispData))
			{
				// Flow downwards
				pDispData->ScenePixels[x + (y + 1) * pDispData->Width] = 1;
				pDispData->ScenePixels[x + y * pDispData->Width] = 0;
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
					pDispData->ScenePixels[x + firstDirection + (y + 1) * pDispData->Width] = 1;
					pDispData->ScenePixels[x + y * pDispData->Width] = 0;
				}
				else if (CanSnowFlowInto(x + secondDirection, y + 1, pDispData) && CanSnowFlowInto(
					x + secondDirection, y, pDispData))
				{
					pDispData->ScenePixels[x + secondDirection + (y + 1) * pDispData->Width] = 1;
					pDispData->ScenePixels[x + y * pDispData->Width] = 0;
				}
			}
		}
	}
}