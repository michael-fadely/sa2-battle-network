#pragma once
#include <LazyTypedefs.h>

struct MemStruct
{
	// System variables are global game settings
	struct SystemVars {
		// The current Game State (loading, ingame, paused, etc)
		uchar GameState;
		// The player number that last paused the game
		uchar PlayerPaused;
		// The current 2Player mode.
		// 0 is single player, 1 is three battles, 2 is one battle.
		uchar TwoPlayerMode;
		// Split screen mode.
		// The number indicates the number of screens rendered.
		uchar SplitscreenMode;
		// The current pause menu selection
		uchar PauseSelection;
	} system;

	// Menu varaibles are for ingame menus of any description
	struct MenuVars
	{
		// The current parent menu and its sub menu
		uint main, sub;
		uchar P2Start;
		uchar PlayerReady[2];

		uint StageSelection2P[2];
		uchar BattleSelection;
		uint  CharacterSelection[2];

		char CharacterSelected[2];

		char AltCharacterSonic, AltCharacterShadow;
		char AltCharacterTails, AltCharacterEggman;
		char AltCharacterKnuckles, AltCharacterRouge;

		// Battle Options button selected on stage select
		char BattleOptionsButton;
		// Battle Options array
		char BattleOptions[4];
		// Battle Options menu selection
		char BattleOptionsSelection;
		// Battle Options back button is selected
		char BattleOptionsBackSelected;
	} menu;

	// Gameplay variables for things only seen within the stage
	struct GameplayVars
	{
		// Ingame time.
		// First index is minutes, followed by seconds and centiseconds.
		char TimerMinutes, TimerSeconds, TimerFrames;
		// Ring count.
		// First index is Player 1, second index is Player 2.
		ushort RingCount[2];
		// Current stage.
		uchar CurrentLevel;

		char P1SpecialAttacks[3];
		char P2SpecialAttacks[3];

		char TimeStopMode;
	} game;
};
