#pragma once

#include <cstdint>

const double BAMS = (65536.0 / 360.0);

// Converts degrees to BAMs
template <typename T>
inline uint32_t toBAMS(T degrees)
{
	return (uint32_t)(degrees * BAMS);
}

// Converts BAMS to degrees
template <typename T>
inline double fromBAMS(T bams)
{
	return (double)(bams / BAMS);
}

