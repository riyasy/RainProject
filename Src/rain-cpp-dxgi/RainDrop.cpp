#include "RainDrop.h"

rain_drop::rain_drop(float windowWidth, float windowHeight, rain_drop_type type): window_width_(windowWidth),
	window_height_(windowHeight), type_(type)
{
	initialize();
}

bool rain_drop::did_drop_land() const
{
	return landed_drop_;
}

bool rain_drop::should_be_erased_and_deleted() const
{
	return frames_for_splatter_ > 50;
}

void rain_drop::move_to_new_position()
{
	if (landed_drop_)
	{
		return;
	}

	// Update the position of the raindrop
	ellipse_.point.x += velocity_x_;
	ellipse_.point.y += velocity_y_;


	// Apply gravity
	if (type_ == rain_drop_type::splatter)
	{
		velocity_y_ += gravity_;
	}


	if (type_ == rain_drop_type::main_drop)
	{
		if (ellipse_.point.y + ellipse_.radiusY >= window_height_)
		{
			ellipse_.point.y = window_height_;
			landed_drop_ = true;
			for (int i = 0; i < 4; i++)
			{
				auto splatter = new rain_drop(window_width_, window_height_, rain_drop_type::splatter);
				float xSpeed = get_random_number(-1, 3);
				xSpeed = xSpeed == 0 ? 0.75f : xSpeed;
				float ySpeed = 10 / (xSpeed * 3.0f);

				splatter->update_position_and_speed(ellipse_.point.x, ellipse_.point.y, xSpeed, ySpeed);
				splatters_.push_back(splatter);
			}
		}
	}

	// Bounce off the edges for Splatter type
	if (type_ == rain_drop_type::splatter)
	{
		if (ellipse_.point.x + ellipse_.radiusX > window_width_ || ellipse_.point.x - ellipse_.radiusX < 0)
		{
			velocity_x_ = -velocity_x_;
		}

		if (ellipse_.point.y + ellipse_.radiusY > window_height_)
		{
			ellipse_.point.y = window_height_ - ellipse_.radiusY; // Keep the ellipse within bounds
			velocity_y_ = -velocity_y_ * bounce_damping_; // Bounce with damping
			splash_count_++;
		}

		if (ellipse_.point.y - ellipse_.radiusY < 0)
		{
			ellipse_.point.y = ellipse_.radiusY; // Keep the ellipse within bounds
			velocity_y_ = -velocity_y_; // Reverse the direction if it hits the top edge
		}
	}
}

void rain_drop::update_position_and_speed(FLOAT x, FLOAT y, float xSpeed, float ySpeed)
{
	ellipse_.point.x = x;
	ellipse_.point.y = y;
	velocity_x_ = xSpeed;
	velocity_y_ = ySpeed;
}

void rain_drop::draw(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush)
{
	//D2D1_RECT_F rect;
	//rect.top = 0;
	//rect.left = 0;
	//rect.right = windowWidth;
	//rect.bottom = windowHeight;
	//dc->DrawRectangle(&rect, pBrush);

	if (!landed_drop_ && ellipse_.point.y >= 0 && ellipse_.point.x >= 0)
	{
		if (type_ == rain_drop_type::main_drop)
		{
			//dc->FillEllipse(ellipse, pBrush);
			D2D1_POINT_2F prevPoint;
			prevPoint.x = ellipse_.point.x - 3 * velocity_x_;
			prevPoint.y = ellipse_.point.y - 3 * velocity_y_;
			dc->DrawLine(prevPoint, ellipse_.point, pBrush);
		}
		else if (type_ == rain_drop_type::splatter)
		{
			if (splash_count_ < 3)
			{
				dc->FillEllipse(ellipse_, pBrush);
			}
		}
	}
	if (!splatters_.empty())
	{
		frames_for_splatter_++;
	}
	for (const auto drop : splatters_)
	{
		drop->draw(dc, pBrush);
		drop->move_to_new_position();
	}
}

rain_drop::~rain_drop()
{
	for (const auto drop : splatters_)
	{
		delete drop;
	}
}

int rain_drop::get_random_number(int x, int y)
{
	// Ensure that x is less than or equal to y
	if (x > y)
	{
		std::swap(x, y);
	}
	return std::rand() % (y - x + 1) + x;
}

void rain_drop::initialize()
{
	// Initialize position and size
	ellipse_.point.x = static_cast<float>(get_random_number(-window_width_, window_width_));
	ellipse_.point.y = static_cast<float>((get_random_number(-window_height_, 0) / 10) * 10);
	//ellipse.point.x = 0;
	//ellipse.point.y = 0;


	if (type_ == rain_drop_type::main_drop)
	{
		ellipse_.radiusX = 1;
		ellipse_.radiusY = 1;
	}
	else
	{
		ellipse_.radiusX = 2.0f;
		ellipse_.radiusY = 2.0f;
	}

	// Initialize velocity and physics parameters
	velocity_x_ = 3.0f;
	velocity_y_ = 15.0f;
	gravity_ = 0.4f;
	bounce_damping_ = 1.0f;
}
