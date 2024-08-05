#include "RainDrop.h"
#include <wrl/client.h>
#include <algorithm>


Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> RainDrop::DropColorBrush;
std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> RainDrop::PrebuiltSplatterOpacityBrushes;
//Microsoft::WRL::ComPtr < ID2D1StrokeStyle1> RainDrop::strokeStyleFixedThickness;
float RainDrop::Gravity = 0.3f;
float RainDrop::BounceDamping = 0.9f;
float RainDrop::ScaleFactor = 1.0f;
int RainDrop::WindowWidth;
int RainDrop::WindowHeight;

RainDrop::RainDrop(const int xVelocity,
                   const RainDropType type):
	VelocityX(xVelocity * ScaleFactor),
	Type(type)
{
	Initialize();
}

void RainDrop::Initialize()
{
	// Initialize position and size
	Ellipse.point.x = static_cast<float>(RandomGenerator::GetInstance().GenerateInt(-WindowWidth, WindowWidth));

	const int y = (RandomGenerator::GetInstance().GenerateInt(-WindowHeight, 0) / 10) * 10;
	Ellipse.point.y = static_cast<float>(y);

	if (Type == RainDropType::MainDrop)
	{
		// Create splatters with radius ranging from 0.2 to 0.7 pixels
		Ellipse.radiusX = (RandomGenerator::GetInstance().GenerateInt(2, 7) / 10.0f) * ScaleFactor;
		Ellipse.radiusY = Ellipse.radiusX;
	}
	else
	{
		// Create splatters with radius ranging from 1.0 to 2.0 pixels
		Ellipse.radiusX = (RandomGenerator::GetInstance().GenerateInt(10, 20) / 10.0f) * ScaleFactor;
		Ellipse.radiusY = Ellipse.radiusX;
	}

	// Initialize velocity and physics parameters
	VelocityY = 15.0f * ScaleFactor;

	dropTrailFactor = RandomGenerator::GetInstance().GenerateInt(2, 6);
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
	return CurrentFrameCountForSplatter >= MAX_SPLUTTER_FRAME_COUNT_;
}

void RainDrop::MoveToNewPosition()
{
	if (TouchedGround)
	{
		return;
	}

	// Update the position of the raindrop
	Ellipse.point.x += VelocityX;
	Ellipse.point.y += VelocityY;

	// Apply gravity
	if (Type == RainDropType::Splatter)
	{
		VelocityY += (Gravity * ScaleFactor);
		float velocityXdamp = 0.005f * ScaleFactor;
		if (VelocityX > 0)
		{
			VelocityX = (std::max)(0.0f, VelocityX - velocityXdamp);
		}
		else if (VelocityX < 0)
		{
			VelocityX = (std::min)(0.0f, VelocityX + velocityXdamp);
		}
	}


	if (Type == RainDropType::MainDrop)
	{
		if (Ellipse.point.y + Ellipse.radiusY >= WindowHeight)
		{
			Ellipse.point.y = static_cast<float>(WindowHeight);
			TouchedGround = true;
			for (int i = 0; i < MAX_SPLATTER_PER_RAINDROP_; i++)
			{
				auto splatter = new RainDrop(0, RainDropType::Splatter);

				int parentXSpeedUnscaled = (VelocityX / ScaleFactor);
				int speed10x = RandomGenerator::GetInstance().GenerateInt(
					-15 + parentXSpeedUnscaled, -6, 6, 15 + parentXSpeedUnscaled);
				float xSpeed = ScaleFactor * (speed10x / 10.0f);

				//wchar_t text_buffer[20] = { 0 }; //temporary buffer
				//swprintf(text_buffer, _countof(text_buffer), L"%d\n", speed10x); // convert
				//OutputDebugString(text_buffer); // print

				float ySpeed = (3.0f * ScaleFactor * ScaleFactor) / xSpeed;

				splatter->SetPositionAndSpeed(Ellipse.point.x, Ellipse.point.y, xSpeed, ySpeed);
				Splatters.push_back(splatter);
			}
		}
	}

	// Bounce off the edges for Splatter type
	if (Type == RainDropType::Splatter)
	{
		if (Ellipse.point.x + Ellipse.radiusX > WindowWidth || Ellipse.point.x - Ellipse.radiusX < 0)
		{
			VelocityX = -VelocityX;
		}

		if (Ellipse.point.y + Ellipse.radiusY > WindowHeight)
		{
			Ellipse.point.y = WindowHeight - Ellipse.radiusY; // Keep the ellipse within bounds
			VelocityY = -VelocityY * BounceDamping; // Bounce with damping
			SplatterBounceCount++;
		}

		if (Ellipse.point.y - Ellipse.radiusY < 0)
		{
			Ellipse.point.y = Ellipse.radiusY; // Keep the ellipse within bounds
			VelocityY = -VelocityY; // Reverse the direction if it hits the top edge
		}
	}
}


void RainDrop::Draw(ID2D1DeviceContext* dc)
{
	//D2D1_RECT_F rect;
	//rect.top = 0;
	//rect.left = 0;
	//rect.right = WindowWidth;
	//rect.bottom = WindowHeight;
	//dc->DrawRectangle(&rect, DropColorBrush.Get());


	if (!TouchedGround && Ellipse.point.y >= 0 && Ellipse.point.x >= 0)
	{
		if (Type == RainDropType::MainDrop)
		{
			//dc->FillEllipse(ellipse, pBrush);
			D2D1_POINT_2F prevPoint;
			prevPoint.x = Ellipse.point.x - dropTrailFactor * VelocityX;
			prevPoint.y = Ellipse.point.y - dropTrailFactor * VelocityY;
			dc->DrawLine(prevPoint, Ellipse.point, DropColorBrush.Get(), Ellipse.radiusX);
		}
	}
	if (!Splatters.empty())
	{
		for (const auto drop : Splatters)
		{
			drop->DrawSplatter(dc, PrebuiltSplatterOpacityBrushes[CurrentFrameCountForSplatter].Get());
			drop->MoveToNewPosition();
		}
		CurrentFrameCountForSplatter++;
	}
}

void RainDrop::DrawSplatter(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush) const
{
	if (!TouchedGround && Ellipse.point.y >= 0 && Ellipse.point.x >= 0)
	{
		if (Type == RainDropType::Splatter)
		{
			if (SplatterBounceCount < MAX_SPLATTER_BOUNCE_COUNT_)
			{
				dc->FillEllipse(Ellipse, pBrush);
			}
		}
	}
}


void RainDrop::SetPositionAndSpeed(const FLOAT x, const FLOAT y, const float xSpeed, const float ySpeed)
{
	Ellipse.point.x = x;
	Ellipse.point.y = y;
	VelocityX = xSpeed;
	VelocityY = ySpeed;
}

void RainDrop::SetRainColor(ID2D1DeviceContext* dc, const COLORREF color)
{
	// Extract the individual color components from COLORREF
	float red = static_cast<float>(GetRValue(color)) / 255.0f;
	float green = static_cast<float>(GetGValue(color)) / 255.0f;
	float blue = static_cast<float>(GetBValue(color)) / 255.0f;

	dc->CreateSolidColorBrush(D2D1::ColorF(red, green, blue, 1.0f), DropColorBrush.GetAddressOf());

	if (!PrebuiltSplatterOpacityBrushes.empty())
	{
		PrebuiltSplatterOpacityBrushes.clear();
	}
	for (int i = 0; i < MAX_SPLUTTER_FRAME_COUNT_; i++)
	{
		float alpha = 1.0f - static_cast<float>(i) / static_cast<float>(MAX_SPLUTTER_FRAME_COUNT_);
		const D2D1_COLOR_F splatterColor = D2D1::ColorF(red, green, blue, alpha);
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> splatterColorBrush;
		dc->CreateSolidColorBrush(splatterColor, splatterColorBrush.GetAddressOf());
		PrebuiltSplatterOpacityBrushes.push_back(splatterColorBrush);
	}
}

//void RainDrop::SetRainColor(ID2D1Factory2* factory, ID2D1DeviceContext* dc, COLORREF color)
//{
//	SetRainColor(dc, color);
//
//	HRESULT hr = factory->CreateStrokeStyle(
//		D2D1::StrokeStyleProperties1(
//			D2D1_CAP_STYLE_FLAT,
//			D2D1_CAP_STYLE_FLAT,
//			D2D1_CAP_STYLE_FLAT,
//			D2D1_LINE_JOIN_MITER,
//			10.0f,
//			D2D1_DASH_STYLE_SOLID,
//			0.0f, D2D1_STROKE_TRANSFORM_TYPE_FIXED),
//		nullptr, 0, strokeStyleFixedThickness.GetAddressOf());
//}

void RainDrop::SetWindowBounds(int windowWidth, int windowHeight, float scaleFactor)
{
	WindowWidth = windowWidth;
	WindowHeight = windowHeight;
	ScaleFactor = scaleFactor;
}
