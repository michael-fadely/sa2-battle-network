#pragma once

#include <cstdint>
#include "check_size.h"

struct MemStruct
{
	struct SystemVars
	{
		int16_t GameState;
		uint8_t PauseSelection;
	} system;

	struct MenuVars
	{
		int32_t SubMenu;
		int32_t  PlayerReady[2];

		int32_t  StageSelection2P[2];
		uint8_t  BattleSelection;
		int32_t CharacterSelection[2];
		int8_t   CharacterSelected[2];

		CHECK_SIZE(BattleSelection);

		CharSelectThing CharSelectThings[CharSelectThings_Length];

		// Battle Options button selected on stage select
		int32_t BattleOptionsButton;
		int8_t BattleOptions[4];
		int8_t BattleOptionsSelection;
		// Battle Options back button is selected
		int8_t BattleOptionsBack;

		CHECK_SIZE(BattleOptionsButton);
		CHECK_SIZE(BattleOptionsSelection);
		CHECK_SIZE(BattleOptionsBack);
	} menu;

	// Gameplay variables for things only seen within the stage
	struct GameplayVars
	{
		int8_t TimerMinutes;
		int8_t TimerSeconds;
		int8_t TimerFrames;
		int8_t SpecialAttacks[2][3];
		int8_t TimeStopped;

		CHECK_SIZE(TimerMinutes);
		CHECK_SIZE(TimerSeconds);
		CHECK_SIZE(TimerFrames);
		CHECK_SIZE(TimeStopped);
	} game;
};
