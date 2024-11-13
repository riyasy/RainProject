#pragma once

#include <cmath>
#include <d2d1_1.h>

class Vector2
{
public:
	float x, y;

	Vector2() : x(0.0f), y(0.0f)
	{
	}

	Vector2(const float x, const float y) : x(x), y(y)
	{
	}

	float magSq() const
	{
		return x * x + y * y;
	}

	void setMag(const float mag)
	{
		const float currentMag = std::sqrt(magSq());
		if (currentMag > 0)
		{
			x = (x / currentMag) * mag;
			y = (y / currentMag) * mag;
		}
	}

	D2D1_POINT_2F ToD2DPoint() const
	{
		return D2D1::Point2F(x, y);
	}
};
