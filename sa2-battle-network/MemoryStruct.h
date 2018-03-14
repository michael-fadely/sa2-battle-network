#pragma once

#include <cstdint>
#include "AddressList.h"

struct MemStruct
{
	struct SystemVars
	{
		int16_t GameState;
		uint8_t PauseSelection;
	} system;

	struct MenuVars
	{
		uint32_t SubMenu;
		int32_t  PlayerReady[2];

		int32_t  StageSelection2P[2];
		uint8_t  BattleSelection;
		uint32_t CharacterSelection[2];
		int8_t   CharacterSelected[2];

		CharSelectThing CharSelectThings[CharSelectThings_Length];

		// Battle Options button selected on stage select
		int8_t BattleOptionsButton;
		int8_t BattleOptions[4];
		int8_t BattleOptionsSelection;
		// Battle Options back button is selected
		int8_t BattleOptionsBack;
	} menu;

	// Gameplay variables for things only seen within the stage
	struct GameplayVars
	{
		int8_t TimerMinutes;
		int8_t TimerSeconds;
		int8_t TimerFrames;
		int8_t SpecialAttacks[2][3];
		int8_t TimeStopped;
	} game;
};
