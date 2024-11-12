#pragma once
#include "windef.h"
#include <d2d1.h>
#include <algorithm>


class MathUtil
{
public:
	// Function to check if a point is inside or on the edge of a RECT
	static bool IsPointInRect(const RECT& rect, const Vector2& point)
	{
		// Check if the point is within or on the boundary of the rectangle
		return (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom);
	}

	// Helper function to calculate intersection
	static bool LineIntersect(const D2D1_POINT_2F& p1, const D2D1_POINT_2F& p2, const D2D1_POINT_2F& q1,
	                          const D2D1_POINT_2F& q2, D2D1_POINT_2F& intersection)
	{
		const float A1 = p2.y - p1.y;
		const float B1 = p1.x - p2.x;
		const float C1 = A1 * p1.x + B1 * p1.y;

		const float A2 = q2.y - q1.y;
		const float B2 = q1.x - q2.x;
		const float C2 = A2 * q1.x + B2 * q1.y;

		const float det = A1 * B2 - A2 * B1;
		if (det == 0) return false; // Parallel lines

		intersection.x = (B2 * C1 - B1 * C2) / det;
		intersection.y = (A1 * C2 - A2 * C1) / det;

		return (intersection.x >= (std::min)(p1.x, p2.x) && intersection.x <= (std::max)(p1.x, p2.x) &&
			intersection.y >= (std::min)(p1.y, p2.y) && intersection.y <= (std::max)(p1.y, p2.y));
	}

	static void TrimLineSegment(const RECT& boundRect, const D2D1_POINT_2F& lineStart, const D2D1_POINT_2F& lineEnd,
	                            D2D1_POINT_2F& lineTrimmedStart, D2D1_POINT_2F& lineTrimmedEnd)
	{
		const D2D1_POINT_2F rectPoints[4] = {
			{static_cast<float>(boundRect.left), static_cast<float>(boundRect.top)},
			{static_cast<float>(boundRect.right), static_cast<float>(boundRect.top)},
			{static_cast<float>(boundRect.right), static_cast<float>(boundRect.bottom)},
			{static_cast<float>(boundRect.left), static_cast<float>(boundRect.bottom)}
		};

		lineTrimmedStart = lineStart;
		lineTrimmedEnd = lineEnd;

		for (int i = 0; i < 4; ++i)
		{
			const int next = (i + 1) % 4;
			D2D1_POINT_2F intersection;
			if (LineIntersect(lineStart, lineEnd, rectPoints[i], rectPoints[next], intersection))
			{
				if (!(lineTrimmedStart.x >= boundRect.left && lineTrimmedStart.x <= boundRect.right &&
					lineTrimmedStart.y >= boundRect.top && lineTrimmedStart.y <= boundRect.bottom))
				{
					lineTrimmedStart = intersection;
				}
				else
				{
					lineTrimmedEnd = intersection;
				}
			}
		}

		// Clamp to rect bounds
		lineTrimmedStart.x = std::clamp(lineTrimmedStart.x, static_cast<float>(boundRect.left),
		                                static_cast<float>(boundRect.right));
		lineTrimmedStart.y = std::clamp(lineTrimmedStart.y, static_cast<float>(boundRect.top),
		                                static_cast<float>(boundRect.bottom));
		lineTrimmedEnd.x = std::clamp(lineTrimmedEnd.x, static_cast<float>(boundRect.left),
		                              static_cast<float>(boundRect.right));
		lineTrimmedEnd.y = std::clamp(lineTrimmedEnd.y, static_cast<float>(boundRect.top),
		                              static_cast<float>(boundRect.bottom));
	}

	static RECT SubtractRect(const RECT& monitorRect, const RECT& taskBarRect)
	{
		RECT result = {0, 0, 0, 0};

		// Check if the taskBarRect is on the top edge and spans the full width
		if (taskBarRect.top == monitorRect.top && taskBarRect.right == monitorRect.right && taskBarRect.left ==
			monitorRect.left)
		{
			// Bottom remaining rectangle
			result = {monitorRect.left, taskBarRect.bottom, monitorRect.right, monitorRect.bottom};
		}
		// Check if the taskBarRect is on the bottom edge and spans the full width
		else if (taskBarRect.bottom == monitorRect.bottom && taskBarRect.right == monitorRect.right && taskBarRect.left
			== monitorRect.left)
		{
			// Top remaining rectangle
			result = {monitorRect.left, monitorRect.top, monitorRect.right, taskBarRect.top};
		}
		// Check if the taskBarRect is on the left edge and spans the full height
		else if (taskBarRect.left == monitorRect.left && taskBarRect.top == monitorRect.top && taskBarRect.bottom ==
			monitorRect.bottom)
		{
			// Right remaining rectangle
			result = {taskBarRect.right, monitorRect.top, monitorRect.right, monitorRect.bottom};
		}
		// Check if the taskBarRect is on the right edge and spans the full height
		else if (taskBarRect.right == monitorRect.right && taskBarRect.top == monitorRect.top && taskBarRect.bottom ==
			monitorRect.bottom)
		{
			// Left remaining rectangle
			result = {monitorRect.left, monitorRect.top, taskBarRect.left, monitorRect.bottom};
		}
		return result;
	}

	static RECT NormalizeRect(const RECT& monitorRect, const int top, const int left)
	{
		RECT result = {
			monitorRect.left - left, // left
			monitorRect.top - top, // top
			monitorRect.right - left, // right
			monitorRect.bottom - top // bottom
		};
		return result;
	}

	static Vector2 FindFirstPoint(double length, Vector2 secondPoint, Vector2 vel)
	{
		// Calculate the magnitude of the velocity vector
		double magnitude = std::sqrt(vel.x * vel.x + vel.y * vel.y);

		// Normalize the velocity components to get the unit direction vector
		double unitVx = vel.x / magnitude;
		double unitVy = vel.y / magnitude;

		// Calculate the first point (x1, y1) using the length and direction
		Vector2 firstPoint;
		firstPoint.x = secondPoint.x - length * unitVx;
		firstPoint.y = secondPoint.y - length * unitVy;

		return firstPoint;
	}
};
