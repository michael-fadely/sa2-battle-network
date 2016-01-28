#pragma once
#include "typedefs.h"
#include "AddressList.h"

struct MemStruct
{
	// System variables are global game settings
	struct SystemVars
	{
		// The current Game State (loading, ingame, paused, etc)
		int16 GameState;
		// The player number that last paused the game
		uint8 PlayerPaused;
		// The current 2Player mode.
		// 0 is single player, 1 is three battles, 2 is one battle.
		uint8 TwoPlayerMode;
		// Split screen mode.
		// The number indicates the number of screens rendered.
		uint8 SplitscreenMode;
		// The current pause menu selection
		uint8 PauseSelection;
	} system;

	// Menu varaibles are for ingame menus of any description
	struct MenuVars
	{
		// The current parent menu and its sub menu
		uint32	main, sub;
		uint8	P2Start;
		int32	PlayerReady[2];

		uint32	StageSelection2P[2];
		uint8	BattleSelection;
		uint32	CharacterSelection[2];

		int8	CharacterSelected[2];

		CharSelectThing CharacterSelections[CharacterSelections_Length];

		// Battle Options button selected on stage select
		int8 BattleOptionsButton;
		// Battle Options array
		int8 BattleOptions[4];
		// Battle Options menu selection
		int8 BattleOptionsSelection;
		// Battle Options back button is selected
		int8 BattleOptionsBack;
	} menu;

	// Gameplay variables for things only seen within the stage
	struct GameplayVars
	{
		// Ingame time.
		// First index is minutes, followed by seconds and centiseconds.
		int8 TimerMinutes, TimerSeconds, TimerFrames;
		// Current stage.
		int16 CurrentLevel;

		int8 P1SpecialAttacks[3];
		int8 P2SpecialAttacks[3];

		int8 TimeStopped;
	} game;
};
