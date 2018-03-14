#pragma once

#include <SA2ModLoader.h>
#include "typedefs.h"

// TODO: Move to mod loader

// Gameplay
DataPointer(int8_t, TimeStopped, 0x0174AFF7);
DataArray(int8_t, SpecialActivateTimer, 0x0174AFF3, 2);
DataPointer(int8_t, Pose2PStart_PlayerNum, 0x0174B009);
DataPointer(int, Pose2PStart_Frames_SkyRailMetalHarbor, 0x01DE95C0);
DataPointer(int8_t, Pose2PStart_PlayerNum_dupe, 0x01DE95C4);
DataPointer(int, Pose2PStart_Frames, 0x01DE95C8);
FunctionPointer(Sint32, DamagePlayer, (CharObj1*, CharObj2Base*), 0x00473800);

// System
DataPointer(uint8_t, PlayerPaused, 0x0174AFD7);
DataPointer(uint8_t, PauseSelection, 0x01933EB1);
DataPointer(uint8_t, SplitscreenMode, 0x01DD946C);
DataPointer(uint, FrameCount, 0x0174B038);
DataPointer(uint, FrameCountIngame, (0x0174B038 + 4));
DataPointer(uint, FrameIncrement, 0x1DEB50C);
DataPointer(uint8_t, TimerFrames, 0x0174AFDD); // ADDR_TIME + 0x02
DataPointer(int8_t, Current2PLevelGroup, 0x0174AFDF);
DataPointer(short, NextLevel, 0x01934BEC);

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

#pragma pack(push, 1)
struct CharSelectThing
{
	int8_t Costume;
	int8_t CostumeUnlocked;
	int8_t gap_2;
	int8_t Visible;
	int8_t gap_4[16];
};
#pragma pack(pop)

DataArray(int, RumblePort_A, 0x01DEFDB0, 4);
DataArray(int, RumblePort_B, 0x008ACF78, 4);

// Menu
DataArray(int8_t, CharacterSelected, 0x01D1B8B2, 2);
DataArray(int, PlayerReady, 0x01AEE598, 2);
DataArray(int, StageSelection2P, 0x01D1C060, 2);
DataArray(uint, CharacterSelection, 0x01A3D8E0, 2);
DataArray(uint, CurrentMenu, 0x01D7BB10, 2);
DataPointer(short, CharacterSelectTimer, 0x01AEE5B0);
DataPointer(uint8_t, BattleSelection, 0x01D1B9F4);
DataPointer(uint8_t, P2Start, 0x01A557C4);

DataArray(int8_t, BattleOptions, 0x01D1C080, 4);
DataPointer(uint8_t, BattleOptionsSelection, 0x01D1C084);
DataPointer(int8_t, BattleOptionsBack, 0x01D1C085);
DataPointer(int8_t, BattleOptionsButton, 0x01D1C08C);

// This accounts for the playable characters (standard + extra, 12) plus the 3 Chao.
DataArray(CharSelectThing, CharSelectThings, 0x01D1B8C5, 15);

// Cheats
DataPointer(Bool, CheatsEnabled, 0x01A55770);
DataPointer(Bool, Cheats_GiveMaxRings, 0x01A55774);
DataPointer(Bool, Cheats_GiveAllUpgrades, 0x01A55778);
DataPointer(Bool, Cheats_GiveMaxLives, 0x01A5577C);
DataPointer(Bool, Cheats_ExitStage, 0x01A558A4);
