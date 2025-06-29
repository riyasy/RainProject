#pragma once
#include "SettingsManager.h"

// An abstract class
class CallBackWindow
{
	// Data members of class
public:
	virtual ~CallBackWindow() = default;
	virtual void UpdateParticleCount(int val) = 0;
	virtual void UpdateWindDirection(int val) = 0;
	virtual void UpdateParticleColor(DWORD color) = 0;
	virtual void UpdateParticleType(ParticleType partType) = 0;
	virtual void UpdateLightningFrequency(int val) = 0;
	virtual void UpdateLightningIntensity(int val) = 0;

	// New callback methods for snow wind randomness
	virtual void UpdateEnableSnowWind(bool enabled) = 0;
	virtual void UpdateSnowWindIntensity(int val) = 0;
	virtual void UpdateSnowWindVariability(int val) = 0;
};
