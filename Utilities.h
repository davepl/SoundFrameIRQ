//+-----------------------------------------------------------------------------
//
// DAV - Dave's Audio Visualizer - (c) 2018 Dave Plummer, All Rights Reserved.
//
// File:        Utiltiies.h
//
// Description:
//
//   Functions and helpers available through the project
//
// History:     Sep-11-2018         Davepl      Split off to separate file
//
//------------------------------------------------------------------------------

#pragma once

#define ARRAYSIZE(a)		(sizeof(a)/sizeof(a[0]))		// Returns the number of elements in an array
#define PERIOD_FROM_FREQ(f) (round(1000000 * (1.0 / f)))	// Calculate period in microseconds (us) from frequency in Hz
#define FREQ_FROM_PERIOD(p) (1.0 / p * 1000000)				// Calculate frequency in Hz given the priod in microseconds (us)

// mapFloat
//
// Given an input value x that can range from in_min to in_max, maps return output value between out_min and out_max

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// FPS
// 
// Given a millisecond value for when the last frame took place and the current timestamp returns the number of
// frames per second, as low as 0.  Never exceeds 999 so you can make some width assumptions.

int FPS(unsigned long start, unsigned long end)
{
	unsigned long msDuration = end - start;
	float fpsf = 1.0f / (msDuration / (float)MS_PER_SECOND);
	int FPS = (int)fpsf;
	if (FPS > 999)
		FPS = 999;
	return FPS;
}

