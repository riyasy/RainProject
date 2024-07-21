#pragma once
#include <windows.h>
#include <d2d1.h>
#include <vector>

#pragma comment(lib, "d2d1")


// Enum for RainDrop types
enum class rain_drop_type
{
	main_drop,
	splatter
};

// RainDrop Class
class rain_drop
{
public:
	rain_drop(float windowWidth, float windowHeight, rain_drop_type type);

	bool did_drop_land() const;

	bool should_be_erased_and_deleted() const;

	void move_to_new_position();

	void update_position_and_speed(FLOAT x, FLOAT y, float xSpeed, float ySpeed);

	void draw(ID2D1HwndRenderTarget* pRenderTarget, ID2D1SolidColorBrush* pBrush);

	~rain_drop();

private:
	D2D1_ELLIPSE ellipse_;
	float velocity_x_;
	float velocity_y_;
	float gravity_;
	float bounce_damping_;
	float window_width_;
	float window_height_;
	rain_drop_type type_;
	bool landed_drop_ = false;
	int frames_for_splatter_ = 0;

	std::vector<rain_drop*> splatters_;

	void initialize();
};
