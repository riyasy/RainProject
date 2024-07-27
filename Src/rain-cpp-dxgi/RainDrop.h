#pragma once


#include <d2d1.h>
#include <vector>
#include <dcomp.h>
#include <vector>
#include <wrl/client.h>


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

	void draw(ID2D1DeviceContext* dc, ID2D1SolidColorBrush* pBrush);

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
	int splash_count_ = 0;
	int frames_for_splatter_ = 0;

	std::vector<rain_drop*> splatters_;

	static constexpr int max_splutter_frame_count_ = 50;
	static int get_random_number(float x, float y);
	void initialize();

	static std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> opacity_brushes_;
	static D2D1_COLOR_F get_opacity_brush_as_per_frame_count(int frame_count);
};
