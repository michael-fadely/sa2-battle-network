#pragma once

#include <SA2ModLoader.h>
#include "typedefs.h"

// TODO: Move to mod loader

// Gameplay
DataPointer(char,	TimeStopped,			0x0174AFF7);
DataArray(char,		SpecialActivateTimer,	0x0174AFF3, 2);

// System
DataPointer(uchar,	PlayerPaused,		0x0174AFD7);
DataPointer(uchar,	PauseSelection,		0x01933EB1);
DataPointer(uchar,	SplitscreenMode,	0x01DD946C);
DataPointer(uint,	FrameCount,			0x0174B038);
DataPointer(uint,	FrameCountIngame,	(0x0174B038 + 4));
DataPointer(uint,	FrameIncrement,		0x1DEB50C);
DataPointer(uchar,	TimerFrames,		0x0174AFDD); // ADDR_TIME + 0x02

#pragma pack(push, 1)
struct PolarCoord
{
	Angle angle;
	float distance;
};
#pragma pack(pop)

// Input
DataArray(ControllerData*, ControllerPointers, 0x01DEFB60, 4);
DataArray(PolarCoord, AnalogThings, 0x01DEFBA0, 8);

// Menu
DataArray(char,		CharacterSelected,		0x01D1B8B2, 2);
DataArray(int,		PlayerReady,			0x01AEE598, 2);
DataArray(int,		StageSelection2P,		0x01D1C060, 2);
DataArray(uint,		CharacterSelection,		0x01A3D8E0, 2);
DataArray(uint,		CurrentMenu,			0x01D7BB10, 2);
DataPointer(short,	CharacterSelectTimer,	0x01AEE5B0);
DataPointer(uchar,	BattleSelection,		0x01D1B9F4);
DataPointer(uchar,	P2Start,				0x01A557C4);

DataArray(char,		BattleOptions,			0x01D1C080, 4);
DataPointer(uchar,	BattleOptionsSelection,	0x01D1C084);
DataPointer(char,	BattleOptionsBack,		0x01D1C085);
DataPointer(char,	BattleOptionsButton,	0x01D1C08C);

// TODO: These might all be off by one except for Sonic's...
DataPointer(char, AltCharacterSonic,	0x01D1B8C5);
DataPointer(char, AltCharacterShadow,	0x01D1B8D9);
DataPointer(char, AltCharacterTails,	0x01D1B8ED);
DataPointer(char, AltCharacterEggman,	0x01D1B901);
DataPointer(char, AltCharacterKnuckles,	0x01D1B915);
DataPointer(char, AltCharacterRouge,	0x01D1B929);

#define ADDR_WINDOWACTIVE	0x00401899
#define ADDR_P2INOP			0x0077E88C
