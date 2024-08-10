#pragma once

// An abstract class
class CallBackWindow
{
	// Data members of class
public:
	virtual ~CallBackWindow() = default;
	virtual void UpdateRainDropCount(int val) = 0;
	virtual void UpdateRainDirection(int val) = 0;
	virtual void UpdateRainColor(DWORD color) = 0;
};
