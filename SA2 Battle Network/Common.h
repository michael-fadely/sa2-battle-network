// Main header of DOOM:
#pragma once

#include <LazyTypedefs.h>
#include <LazySleepFor.h>
#include "Globals.h"

// Useful inline stuff:
// Returns millisecs (lazy description is lazy)
const unsigned int millisecs();

// Returns the amount of time passed since TIMER
const unsigned int Duration(const unsigned int timer);
