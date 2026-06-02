#include "SnowFlake.h"
#include "RandomGenerator.h"
#include "MathUtil.h"
#include "FastNoiseLite.h"
#include <algorithm>
#include <array>

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
	Radius = other.Radius;
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
		Radius = other.Radius;
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
	Pos.x = RandomGenerator::GetInstance().GenerateFloat(-SNOW_EDGE_MARGIN * pDisplayData->Width, (1.0f + SNOW_EDGE_MARGIN) * pDisplayData->Width);
	Pos.y = RandomGenerator::GetInstance().GenerateFloat(-pDisplayData->Height / 2.0f, pDisplayData->Height / 1.0f);
	Vel.x = 0.0f;
	Vel.y = RandomGenerator::GetInstance().GenerateFloat(5.0f, 10.0f);

	Radius = RandomGenerator::GetInstance().GenerateFloat(SNOW_MIN_RADIUS, SNOW_MAX_RADIUS);
	Rotation = RandomGenerator::GetInstance().GenerateFloat(0.0f, TWO_PI); // Random initial rotation (in radians directly)
	RotationSpeed = RandomGenerator::GetInstance().GenerateFloat(-2.0f, 2.0f); // rad/s; matches macOS rotationSpeed
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
	Pos.x = RandomGenerator::GetInstance().GenerateFloat(-SNOW_EDGE_MARGIN * pDisplayData->Width, (1.0f + SNOW_EDGE_MARGIN) * pDisplayData->Width);
	Pos.y = -5.0f;
	Vel.x = 0.0f;
	Vel.y = RandomGenerator::GetInstance().GenerateFloat(5.0f, 10.0f);

	// Visual properties
	Radius = RandomGenerator::GetInstance().GenerateFloat(SNOW_MIN_RADIUS, SNOW_MAX_RADIUS);
	Rotation = RandomGenerator::GetInstance().GenerateFloat(0.0f, TWO_PI); // Random initial rotation (in radians)
	RotationSpeed = RandomGenerator::GetInstance().GenerateFloat(-2.0f, 2.0f); // rad/s; matches macOS rotationSpeed
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

	if (pDisplayData->SimpleSnowHeap)
	{
		// Heightmap settling: deposit into the flake's column when it reaches
		// that column's surface; otherwise keep falling.
		if (Pos.x < -SNOW_EDGE_MARGIN * pDisplayData->Width || Pos.x >= (1.0f + SNOW_EDGE_MARGIN) * pDisplayData->Width ||
			Pos.y < -pDisplayData->Height * 0.5f)
		{
			ReSpawn();
			return;
		}
		const int numCols = static_cast<int>(pDisplayData->ColumnHeights.size());
		if (numCols > 0 && Pos.x >= 0.0f && Pos.x < static_cast<float>(pDisplayData->Width))
		{
			int col = static_cast<int>(Pos.x) / pDisplayData->SnowColumnWidth;
			if (col >= numCols) col = numCols - 1;
			const float surfaceY = pDisplayData->Height - pDisplayData->ColumnHeights[col];
			if (Pos.y >= surfaceY)
			{
				const float deposit = Radius * SNOW_DEPOSIT_FACTOR * pDisplayData->ScaleFactor;
				const float maxHeight = pDisplayData->Height * SNOW_MAX_HEIGHT_FRACTION;
				float& h = pDisplayData->ColumnHeights[col];
				h = (std::min)(h + deposit, maxHeight);
				ReSpawn();
			}
		}
		else if (Pos.y >= pDisplayData->Height)
		{
			ReSpawn(); // fell past the bottom in the off-screen side margins
		}
		return;
	}

	if (Pos.x < -SNOW_EDGE_MARGIN * pDisplayData->Width ||
		Pos.x >= (1.0f + SNOW_EDGE_MARGIN) * pDisplayData->Width ||
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

void SnowFlake::GenerateAtlas(ID2D1DeviceContext* dc, DisplayData* pDispData)
{
	// 2x2 grid of SPRITE_SIZE cells: Simple, Crystal (top row), Hexagon, Star.
	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> bmpRT;
	const D2D1_SIZE_F size = D2D1::SizeF(SPRITE_SIZE * 2.0f, SPRITE_SIZE * 2.0f);
	if (FAILED(dc->CreateCompatibleRenderTarget(size, &bmpRT))) return;

	bmpRT->BeginDraw();
	bmpRT->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent

	const float half = SPRITE_SIZE / 2.0f;
	const float s = SPRITE_SIZE;

	// Each shape is clipped to its cell so overflow (e.g. star spikes that exceed
	// the half-cell) does not bleed into neighbouring cells in the atlas.
	bmpRT->PushAxisAlignedClip(D2D1::RectF(0, 0, s, s), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	DrawSimpleSnowflake(bmpRT.Get(), D2D1::Point2F(half, half), SPRITE_BASE_DRAW_SIZE, pDispData);
	bmpRT->PopAxisAlignedClip();

	bmpRT->PushAxisAlignedClip(D2D1::RectF(s, 0, s * 2.0f, s), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	DrawCrystalSnowflake(bmpRT.Get(), D2D1::Point2F(s + half, half), SPRITE_BASE_DRAW_SIZE, pDispData);
	bmpRT->PopAxisAlignedClip();

	bmpRT->PushAxisAlignedClip(D2D1::RectF(0, s, s, s * 2.0f), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	DrawHexagonSnowflake(bmpRT.Get(), D2D1::Point2F(half, s + half), SPRITE_BASE_DRAW_SIZE, pDispData);
	bmpRT->PopAxisAlignedClip();

	bmpRT->PushAxisAlignedClip(D2D1::RectF(s, s, s * 2.0f, s * 2.0f), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	DrawStarSnowflake(bmpRT.Get(), D2D1::Point2F(s + half, s + half), SPRITE_BASE_DRAW_SIZE, pDispData);
	bmpRT->PopAxisAlignedClip();

	bmpRT->EndDraw();

	Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
	if (SUCCEEDED(bmpRT->GetBitmap(&bitmap)))
	{
		pDispData->SnowAtlas = bitmap;
	}
}

void SnowFlake::DrawFallingFlakes(ID2D1DeviceContext3* dc3, const std::vector<SnowFlake>& flakes, DisplayData* pDispData)
{
	if (flakes.empty()) return;

	// Lazily (re)build the colored atlas and the reusable sprite batch.
	if (pDispData->SnowAtlas == nullptr)
	{
		GenerateAtlas(dc3, pDispData);
		if (pDispData->SnowAtlas == nullptr) return;
	}
	if (pDispData->SnowSpriteBatch == nullptr)
	{
		if (FAILED(dc3->CreateSpriteBatch(pDispData->SnowSpriteBatch.GetAddressOf()))) return;
	}

	// Reused scratch buffers (single-threaded; refilled each call, capacity kept).
	static std::vector<D2D1_RECT_F> dests;
	static std::vector<D2D1_RECT_U> srcs;
	static std::vector<D2D1_COLOR_F> colors;
	static std::vector<D2D1_MATRIX_3X2_F> transforms;
	dests.clear(); srcs.clear(); colors.clear(); transforms.clear();

	const float left = static_cast<float>(pDispData->SceneRect.left);
	const float top = static_cast<float>(pDispData->SceneRect.top);
	const D2D1_COLOR_F white = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f); // atlas is pre-colored; no tint

	// Source rects are in atlas pixels (D2D1_RECT_U); each shape is one of the
	// 2x2 cells. Derive cell size from the atlas's actual pixel size (DPI-safe).
	const D2D1_SIZE_U atlasPx = pDispData->SnowAtlas->GetPixelSize();
	const UINT32 cellW = atlasPx.width / 2;
	const UINT32 cellH = atlasPx.height / 2;

	for (const SnowFlake& f : flakes)
	{
		if (!MathUtil::IsPointInRect(pDispData->SceneRectNorm, f.Pos)) continue;

		const float cx = f.Pos.x + left;
		const float cy = f.Pos.y + top;
		const float halfDraw = f.Radius * SNOW_DRAW_SCALE * pDispData->ScaleFactor;
		dests.push_back(D2D1::RectF(cx - halfDraw, cy - halfDraw, cx + halfDraw, cy + halfDraw));

		const int idx = static_cast<int>(f.Shape);
		const UINT32 sx = static_cast<UINT32>(idx % 2) * cellW;
		const UINT32 sy = static_cast<UINT32>(idx / 2) * cellH;
		srcs.push_back(D2D1::RectU(sx, sy, sx + cellW, sy + cellH));

		colors.push_back(white);
		transforms.push_back(D2D1::Matrix3x2F::Rotation(f.Rotation * 180.0f / PI, D2D1::Point2F(cx, cy)));
	}

	if (dests.empty()) return;

	ID2D1SpriteBatch* batch = pDispData->SnowSpriteBatch.Get();
	batch->Clear();
	batch->AddSprites(static_cast<UINT32>(dests.size()), dests.data(), srcs.data(), colors.data(), transforms.data());

	// Sprite batch requires aliased AA; sprite edges are pre-antialiased in the atlas.
	const D2D1_ANTIALIAS_MODE prevAA = dc3->GetAntialiasMode();
	dc3->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	dc3->DrawSpriteBatch(batch, pDispData->SnowAtlas.Get());
	dc3->SetAntialiasMode(prevAA);
}

void SnowFlake::DrawSimpleSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData)
{
	// For simple snowflakes, just draw an ellipse with slight variations
	const float radiusX = 1.0f * size;
	const float radiusY = 0.7f * size;

	// Draw the ellipse
	D2D1_ELLIPSE ellipse = D2D1::Ellipse(center, radiusX, radiusY);
	rt->FillEllipse(ellipse, pDispData->DropColorBrush.Get());
}

void SnowFlake::DrawCrystalSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData)
{
	// Draw a small center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.5f, size * 0.5f);
	rt->FillEllipse(centerCircle, pDispData->DropColorBrush.Get());

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
		rt->DrawLine(center, endPoint, pDispData->DropColorBrush.Get(), size * 0.2f);

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
		rt->DrawLine(midPoint, branch1End, pDispData->DropColorBrush.Get(), size * 0.15f);

		// Second branch
		float branch2Angle = angle - branchAngleOffset;
		float branch2EndX = midX + cos(branch2Angle) * branchLength;
		float branch2EndY = midY + sin(branch2Angle) * branchLength;
		D2D1_POINT_2F branch2End = D2D1::Point2F(branch2EndX, branch2EndY);
		rt->DrawLine(midPoint, branch2End, pDispData->DropColorBrush.Get(), size * 0.15f);
	}
}

void SnowFlake::DrawHexagonSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData)
{
	// Draw a hexagon shape using lines
	constexpr int sides = 6;
	const float radius = size * 2.0f;

	// Stack-allocated array — no heap, no leak risk (sides + 1 = 7)
	std::array<D2D1_POINT_2F, 7> points;

	for (int i = 0; i <= sides; ++i) {
		const float angle = i * TWO_PI / sides;
		points[i] = D2D1::Point2F(
			center.x + radius * std::cos(angle),
			center.y + radius * std::sin(angle)
		);
	}

	// Draw the hexagon outline
	for (int i = 0; i < sides; ++i) {
		rt->DrawLine(points[i], points[i + 1], pDispData->DropColorBrush.Get(), size * 0.2f);
	}

	// Draw inner details (spokes)
	for (int i = 0; i < sides; ++i) {
		rt->DrawLine(
			center,
			points[i],
			pDispData->DropColorBrush.Get(),
			size * 0.15f
		);
	}

	// Draw center circle
	const D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.4f, size * 0.4f);
	rt->FillEllipse(centerCircle, pDispData->DropColorBrush.Get());
}

void SnowFlake::DrawStarSnowflake(ID2D1RenderTarget* rt, D2D1_POINT_2F center, float size, DisplayData* pDispData)
{
	// Draw a small center circle
	D2D1_ELLIPSE centerCircle = D2D1::Ellipse(center, size * 0.4f, size * 0.4f);
	rt->FillEllipse(centerCircle, pDispData->DropColorBrush.Get());

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
		rt->DrawLine(center, endPoint, pDispData->DropColorBrush.Get(), size * 0.15f);

		// Draw small intersecting lines between main spikes
		if (i % 2 == 0) {
			float crossAngle = angle + (TWO_PI / numSpikes / 2);
			float crossX = center.x + cos(crossAngle) * innerRadius;
			float crossY = center.y + sin(crossAngle) * innerRadius;
			D2D1_POINT_2F crossPoint = D2D1::Point2F(crossX, crossY);

			rt->DrawLine(center, crossPoint, pDispData->DropColorBrush.Get(), size * 0.1f);
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

void SnowFlake::SmoothSnowHeap(DisplayData* pDispData)
{
	std::vector<float>& h = pDispData->ColumnHeights;
	const int n = static_cast<int>(h.size());
	if (n < 3) return;

	// Volume-conserving diffusion (matches the macOS SnowSystem::smoothHeightMap):
	// forward then backward, move a fraction of any adjacent-column excess above the
	// threshold into the lower neighbour, so the pile relaxes into organic slopes
	// while total settled snow is preserved (rather than a hard slope clamp).
	const float threshold = SNOW_SMOOTH_THRESHOLD * pDispData->ScaleFactor;
	for (int x = 1; x < n - 1; ++x)
	{
		const float diff = h[x] - h[x - 1];
		if (diff > threshold) { const float f = diff * SNOW_SMOOTH_RATE; h[x] -= f; h[x - 1] += f; }
	}
	for (int x = n - 2; x >= 1; --x)
	{
		const float diff = h[x] - h[x + 1];
		if (diff > threshold) { const float f = diff * SNOW_SMOOTH_RATE; h[x] -= f; h[x + 1] += f; }
	}
}

void SnowFlake::DrawSettledSnowSimple(ID2D1DeviceContext* dc, const DisplayData* pDispData)
{
	const std::vector<float>& h = pDispData->ColumnHeights;
	const int numCols = static_cast<int>(h.size());
	const int width = pDispData->Width;
	const int cellW = pDispData->SnowColumnWidth;
	if (numCols < 1 || width < 1 || cellW < 1) return;

	if (pDispData->Factory == nullptr) return;
	Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
	if (FAILED(pDispData->Factory->CreatePathGeometry(geometry.GetAddressOf()))) return;
	Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
	if (FAILED(geometry->Open(sink.GetAddressOf()))) return;

	const float left = static_cast<float>(pDispData->SceneRect.left);
	const float top = static_cast<float>(pDispData->SceneRect.top);
	const float bottom = top + pDispData->Height;

	// Filled silhouette: left edge -> across the coarse column tops -> right edge.
	sink->BeginFigure(D2D1::Point2F(left, bottom), D2D1_FIGURE_BEGIN_FILLED);
	for (int i = 0; i < numCols; ++i)
	{
		sink->AddLine(D2D1::Point2F(left + static_cast<float>(i * cellW), bottom - h[i]));
	}
	// Extend the last column's height to the right edge, then close along the bottom.
	sink->AddLine(D2D1::Point2F(left + static_cast<float>(width), bottom - h[numCols - 1]));
	sink->AddLine(D2D1::Point2F(left + static_cast<float>(width), bottom));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();

	dc->FillGeometry(geometry.Get(), pDispData->DropColorBrush.Get());
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