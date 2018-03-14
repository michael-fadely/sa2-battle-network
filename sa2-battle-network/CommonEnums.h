#pragma once

#include <cstdint>

struct GameState
{
	enum GameStates : short
	{
		Inactive,
		Loading,
		LoadItems,
		LoadLevel,
		ReturnToMenu_1,
		ReloadCharacter,
		ReturnToMenu_2,
		LoadFinished,
		Exit_1,
		RestartLevel_NoLifeLost,
		Exit_2,
		Exit_3,
		RestartLevel_1,
		NormalRestart,
		NormalExit,
		ReturnRing,
		Ingame,
		Pause,
		MemoryCard,
		GoToNextLevel,
		Unknown_14,
		Unknown_15,
		Unknown_16,
		Unknown_17,
		Unknown_18,
		Unknown_19,
		Unknown_1A,
		Unknown_1B,
		Unknown_1C,
		Unknown_1D,
		Unknown_1E,
		Unknown_1F,
		Unknown_20,
		Unknown_21,
		Unknown_22,
		Unknown_23,
		Unknown_24,
		Unknown_25,
		Unknown_26,
		Unknown_27,
		Unknown_28,
		Unknown_29,
		Unknown_2A,
		Unknown_2B,
		Unknown_2C,
		Unknown_2D,
		Unknown_2E,
		Unknown_2F,
		Unknown_30,
		Unknown_31,
		Unknown_32,
	};
};

struct Menu
{
	enum Menus : uint8_t
	{
		title,
		main,
		story_select,
		stage_select,
		idk_1,
		idk_2,
		settings,
		idk_3,
		sound_test,
		idk_4,
		kart,
		idk_5,
		boss_attack,
		idk_6,
		idk_7,
		emblem,
		battle,
	};
};

// 2P Sub-menus
// Where s is static,
// i is in (intro transition),
// o is out (exit transition)
struct SubMenu2P
{
	enum SubMenus : uint8_t
	{
		s_null,
		i_start,
		s_battlemode,
		s_charsel,
		o_charsel,
		i_stagesel,
		s_stagesel = 8,
		o_stagesel,
		s_battleopt,
		s_start = 13,
		s_ready,
		o_ready,
	};
};