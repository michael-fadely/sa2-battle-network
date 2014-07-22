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
		uchar multiMode;
		// Split screen mode.
		// The number indicates the number of screens rendered.
		uchar splitscreen;
		// The current pause menu selection
		uchar PauseMenuSelection;
	} system;

	// Menu varaibles are for ingame menus of any description
	struct MenuVars
	{
		// The current parent menu and its sub menu
		uint main, sub;
		uchar p2start;
		uchar playerReady[2];

		uint StageSel2P[2];
		uchar BattleModeSel;
		uchar charSelection[2];

		char selectedChar[2];

		char altChar[6], atMenu[2];

		// Battle Options button selected on
		// stage select
		char BattleOptButton;
		// Battle Options array
		char battleOpt[4];
		// Battle Options menu selection
		char BattleOptSelection;
		// Battle Options back button is selected
		char BattleOptBack;
	} menu;

	// Gameplay variables for things only seen within the stage
	struct GameplayVars
	{
		// Ingame time.
		// First index is minutes, followed by seconds and centiseconds.
		char Time[3];
		// Ring count.
		// First index is Player 1, second index is Player 2.
		ushort rings[2];
		// Current stage.
		uchar stage;

		char p1specials[3];
		char p2specials[3];

		char TimeStop;
	} game;
};
