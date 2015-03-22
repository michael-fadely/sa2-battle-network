#pragma once

#include <SA2ModLoader.h>

#pragma region Gameplay

#define ADDR_PLAYER1		0x01DEA6E0	// MainCharacter[0]
#define ADDR_PLAYER2		0x01DEA6E4	// MainCharacter[1]

#define ADDR_P1RINGS		0x0174B028	// RingCount[0]
#define ADDR_P2RINGS		0x0174B02A	// RingCount[1]

#define ADDR_P1SPECIALS		0x0174AFED	// P1SpecialAttacks
#define ADDR_P2SPECIALS		0x0174AFF0	// P2SpecialAttacks
#define ADDR_TIMESTOP		0x0174AFF7

DataPointer(char, TimeStopMode, ADDR_TIMESTOP);

#pragma endregion

#pragma region System
#define ADDR_PLYPAUSED		0x0174AFD7
#define ADDR_2PMODE			0x0174AFDE	// TwoPlayerMode
#define ADDR_PAUSESEL		0x01933EB1
#define ADDR_GAMESTATE		0x01934BE0

#define ADDR_STAGE			0x01934B70	// CurrentLevel
#define ADDR_SPLITSCREEN	0x01DD946C

#define ADDR_WINDOWACTIVE	0x00401899
#define ADDR_FRAMECOUNT		0x0174B038

#define ADDR_TIME			0x0174AFDB	// TimerMinutes, TimerSeconds

DataPointer(unsigned char, PlayerPaused, ADDR_PLYPAUSED);
DataPointer(unsigned char, PauseSelection, ADDR_PAUSESEL);
DataPointer(unsigned char, GameState, ADDR_GAMESTATE);
DataPointer(unsigned char, SplitscreenMode, ADDR_SPLITSCREEN);
DataPointer(unsigned int, FrameCount, ADDR_FRAMECOUNT);
DataPointer(unsigned char, TimerFrames, 0x0174AFDD); // ADDR_TIME + 0x02
#pragma endregion

#pragma region Input

#define ADDR_INPUT_PTRS		0x01DEFB60
#define ADDR_P2INOP			0x0077E88C

DataArray(ControllerData*, ControllerPtr, ADDR_INPUT_PTRS, 4);
DataPointer(ControllerData*, ControllerPtr1, &ControllerPtr[0]);
DataPointer(ControllerData*, ControllerPtr2, &ControllerPtr[1]);
DataPointer(ControllerData*, ControllerPtr3, &ControllerPtr[2]);
DataPointer(ControllerData*, ControllerPtr4, &ControllerPtr[3]);

#pragma endregion

#pragma region Menu

#pragma region Battle
#define ADDR_MENU			0x01D7BB10
#define ADDR_SUBMENU		0x01D7BB14
#define ADDR_P2START		0x01A557C4
#define ADDR_CHOSENTIMER	0x01AEE5B0
#define ADDR_P1READY		0x01AEE598
#define ADDR_P2READY		0x01AEE59C
#define ADDR_P1CHARSEL		0x01A3D8E0
#define ADDR_P2CHARSEL		0x01A3D8E4
#define ADDR_P1CHARCHOSEN	0x01D1B8B2
#define ADDR_P2CHARCHOSEN	0x01D1B8B3
#define ADDR_2PMENUSEL		0x01D1B9F4
#define ADDR_STAGESELV		0x01D1C060
#define ADDR_STAGESELH		0x01D1C064

DataArray(unsigned int, CurrentMenu, ADDR_MENU, 2);
DataPointer(unsigned char, P2Start, ADDR_P2START);
DataPointer(short, CharacterSelectTimer, ADDR_CHOSENTIMER);
DataArray(int, PlayerReady, ADDR_P1READY, 2);
DataArray(unsigned int, CharacterSelection, ADDR_P1CHARSEL, 2);
DataArray(char, CharacterSelected, ADDR_P1CHARCHOSEN, 2);
DataPointer(unsigned char, BattleSelection, ADDR_2PMENUSEL);
DataArray(int, StageSelection2P, ADDR_STAGESELV, 2);
#pragma endregion

#pragma region Battle Options
#define ADDR_BATTOPT		0x01D1C080
#define ADDR_BATTOPT_SEL	0x01D1C084
#define ADDR_BATTOPT_BAK	0x01D1C085
#define ADDR_BATTOPT_BTN	0x01D1C08C

DataArray(char, BattleOptions, ADDR_BATTOPT, 4);
DataPointer(unsigned char, BattleOptionsSelection, ADDR_BATTOPT_SEL);
DataPointer(char, BattleOptionsBack, ADDR_BATTOPT_BAK);
DataPointer(char, BattleOptionsButton, ADDR_BATTOPT_BTN);
#pragma endregion

#pragma region Alt Character Selection
#define ADDR_ALTSONIC		0x01D1B8C5
#define ADDR_ALTSHADOW		0x01D1B8D9
#define ADDR_ALTTAILS		0x01D1B8ED
#define ADDR_ALTEGGMAN		0x01D1B901
#define ADDR_ALTKNUX		0x01D1B915
#define ADDR_ALTROUGE		0x01D1B929

DataPointer(char, AltCharacterSonic, ADDR_ALTSONIC);
DataPointer(char, AltCharacterShadow, ADDR_ALTSHADOW);
DataPointer(char, AltCharacterTails, ADDR_ALTTAILS);
DataPointer(char, AltCharacterEggman, ADDR_ALTEGGMAN);
DataPointer(char, AltCharacterKnuckles, ADDR_ALTKNUX);
DataPointer(char, AltCharacterRouge, ADDR_ALTROUGE);
#pragma endregion

#pragma endregion