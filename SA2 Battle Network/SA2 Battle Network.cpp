// SA2 Battle Network.cpp : Defines the exported functions for the DLL application.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <SA2ModLoader.h>

#include "Initialize.h"


extern "C"				// Required for proper export
__declspec(dllexport)	// This data is being exported from this DLL
ModInfo SA2ModInfo = {
	ModLoaderVer,		// Struct version
	Init_t,				// Initialization function
	NULL, 0,			// List of Patches & Patch Count
	NULL, 0,			// List of Jumps & Jump Count
	NULL, 0,			// List of Calls & Call Count
	NULL, 0,			// List of Pointers & Pointer Count
};
