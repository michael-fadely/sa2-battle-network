#pragma once

struct GameState
{
	enum GameStates : short
	{
		Inactive,
		Loading,
		ItemCloner,
		CombineLevels,
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
		ReturnToMenu_3,
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
	enum Menus : unsigned char
	{
		TITLE,
		MAIN,
		STORYSEL,
		STAGESEL,
		IDK_1,
		IDK_2,
		SETTINGS,
		IDK_3,
		SOUNDTEST,
		IDK_4,
		KART,
		IDK_5,
		BOSSATK,
		IDK_6,
		IDK_7,
		EMBLEM,
		BATTLE,
	};
};

// 2P Sub-menus
// Where S is Static,
// I is In (intro transition),
// O is Out (exit transition)
struct SubMenu2P
{
	enum SubMenus : unsigned char
	{
		S_NULL,
		I_START,
		S_BATTLEMODE,
		S_CHARSEL,
		O_CHARSEL,
		I_STAGESEL,
		S_STAGESEL = 8,
		O_STAGESEL,
		S_BATTLEOPT,
		S_START = 13,
		S_READY,
		O_READY,
	};
};