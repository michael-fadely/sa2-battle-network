#pragma once
#include "typedefs.h"
#include "AddressList.h"

struct MemStruct
{
	struct SystemVars
	{
		int16 GameState;
		uint8 PauseSelection;
	} system;

	struct MenuVars
	{
		uint32 SubMenu;
		int32 PlayerReady[2];

		uint32	StageSelection2P[2];
		uint8	BattleSelection;
		uint32	CharacterSelection[2];
		int8	CharacterSelected[2];

		CharSelectThing CharSelectThings[CharSelectThings_Length];

		// Battle Options button selected on stage select
		int8 BattleOptionsButton;
		int8 BattleOptions[4];
		int8 BattleOptionsSelection;
		// Battle Options back button is selected
		int8 BattleOptionsBack;
	} menu;

	// Gameplay variables for things only seen within the stage
	struct GameplayVars
	{
		// Ingame time.
		int8 TimerMinutes, TimerSeconds, TimerFrames;
		int8 SpecialAttacks[2][3];
		int8 TimeStopped;
	} game;
};
