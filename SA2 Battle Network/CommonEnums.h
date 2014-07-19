#pragma once

struct GameState
{
	enum States : char
	{
		INACTIVE,
		LOADING,
		ITEM_CLONER,
		COMBINE_LEVELS,
		RETURN_TO_MENU_1,
		RELOAD_CHARACTER,
		RETURN_TO_MENU_2,
		LOAD_FINISHED,
		EXIT_1,
		RESTART_LEVEL_NOLIFE,
		EXIT_2,
		EXIT_3,
		RESTART_LEVEL_1,
		NORMAL_RESTART,
		NORMAL_EXIT,
		RETURN_RING,
		INGAME,
		PAUSE,
		MEMCARD,
		RETURN_TO_MENU_3
	};
};

struct Menu
{
	enum Menus : char
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
	enum SubMenus : char
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