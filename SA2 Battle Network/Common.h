// TODO: DESTROY!
#pragma once

#include <LazyTypedefs.h>
#include <LazySleepFor.h>
#include "Globals.h"

// GetTickCount() wrapper.
uint32 Millisecs();

// Returns elapsed time since the parameter "timer"
uint32 Duration(const uint32 timer);
