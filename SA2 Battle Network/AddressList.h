#pragma once
#include <SA2ModLoader.h>

// Gameplay:
//#define ADDR_PLAYER1		0x01DEA6E0	// MainCharacter[0]
//#define ADDR_PLAYER2		0x01DEA6E4	// MainCharacter[1]

//#define ADDR_P1RINGS		0x0174B028	// RingCount[0]
//#define ADDR_P2RINGS		0x0174B02A	// RingCount[1]

//#define ADDR_P1SPECIALS		0x0174AFED	// P1SpecialAttacks
//#define ADDR_P2SPECIALS		0x0174AFF0	// P2SpecialAttacks
//#define ADDR_TIMESTOP		0x0174AFF7
DataPointer(char, TimeStopMode, 0x0174AFF7);

// System:
//#define ADDR_PLYPAUSED		0x0174AFD7
DataPointer(char, PlayerPaused, 0x0174AFD7);

//#define ADDR_2PMODE			0x0174AFDE	// TwoPlayerMode
//#define ADDR_PAUSESEL		0x01933EB1
DataPointer(char, PauseSelection, 0x01933EB1);

//#define ADDR_GAMESTATE		0x01934BE0
DataPointer(char, GameState, 0x01934BE0);

//#define ADDR_STAGE			0x01934B70	// CurrentLevel
//#define ADDR_SPLITSCREEN	0x01DD946C
DataPointer(char, SplitscreenMode, 0x01DD946C);

#define ADDR_WINDOWACTIVE	0x00401899
#define ADDR_FRAMECOUNT		0x0174B038
DataPointer(unsigned int, FrameCount, 0x0174B038);

//#define ADDR_TIME			0x0174AFDB	// TimerMinutes,
										// TimerSeconds,

// Input:
#define ADDR_P1INPUT		0x01DEFB60
#define ADDR_P2INPUT		0x01DEFB64
#define ADDR_P2INOP			0x0077E88C

// Menu:
//#define ADDR_MENU			0x01D7BB10
//#define ADDR_SUBMENU		0x01D7BB14
DataArray(unsigned int, CurrentMenu, 0x01D7BB10, 2);

//#define ADDR_P2START		0x01A557C4
DataPointer(char, P2Start, 0x01A557C4);
//#define ADDR_CHOSENTIMER	0x01AEE5B0
DataPointer(short, CharacterSelectTimer, 0x01AEE5B0);
//#define ADDR_P1READY		0x01AEE598
//#define ADDR_P2READY		0x01AEE59C
DataArray(int, PlayerReady, 0x01AEE598, 2);
//#define ADDR_P1CHARSEL		0x01A3D8E0
//#define ADDR_P2CHARSEL		0x01A3D8E4
DataArray(int, CharacterSelection, 0x01A3D8E0, 2);
//#define ADDR_P1CHARCHOSEN	0x01D1B8B2
//#define ADDR_P2CHARCHOSEN	0x01D1B8B3
DataArray(char, CharacterSelected, 0x01D1B8B2, 2);
//#define ADDR_2PMENUSEL		0x01D1B9F4
DataPointer(char, BattleSelect, 0x01D1B9F4);
//#define ADDR_STAGESELV		0x01D1C060
//#define ADDR_STAGESELH		0x01D1C064
DataArray(int, StageSelect2P, 0x01D1C060, 2);

//#define ADDR_BATTOPT		0x01D1C080
DataArray(char, BattleOptions, 0x01D1C080, 4);
//#define ADDR_BATTOPT_SEL	0x01D1C084
DataPointer(char, BattleOptionSelection, 0x01D1C084);
//#define ADDR_BATTOPT_BAK	0x01D1C085
DataPointer(char, BattleOptionBackSelected, 0x01D1C085);
//#define ADDR_BATTOPT_BTN	0x01D1C08C
DataPointer(char, BattleOptionsButton, 0x01D1C08C);

#define ADDR_ALTSONIC		0x01D1B8C5
#define ADDR_ALTSHADOW		0x01D1B8D9
#define ADDR_ALTTAILS		0x01D1B8ED
#define ADDR_ALTEGGMAN		0x01D1B901
#define ADDR_ALTKNUX		0x01D1B915
#define ADDR_ALTROUGE		0x01D1B929