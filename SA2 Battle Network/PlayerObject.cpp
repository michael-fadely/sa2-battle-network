#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Common.h"
#include "LazyMemory.h"
#include "GameObject.h"

#include "PlayerObject.h"

using namespace std;

// Constructor/Destructor
PlayerObject::PlayerObject(uint address) : GameObject(address), AbstractPlayer({})
{
	lastPointer = 0;
	pointerEval();
}

// Functions

void PlayerObject::pointerEval()
{
	if (CheckInitialized())
	{
		if (lastPointer != ptrAddress)
		{
			cout << "<>\tRe-initializing Player..." << endl;
			// Character Object 1
			ReadMemory((ptrAddress + Master::objData1), &objData1ptr, sizeof(int));
			// Character Object 2
			ReadMemory((ptrAddress + Master::objData2), &objData2ptr, sizeof(int));
			
			lastPointer = ptrAddress;
		}
	}
}

// Player variable updater
// Reads all the variables from memory and stores in the object
void PlayerObject::read()
{
	pointerEval();

	if (exists)
	{
		// Character Object 1
		// Action & Status
		ReadMemory((objData1ptr + objData1::Action), &Action, sizeof(char));
		ReadMemory((objData1ptr + objData1::Status), &Status, sizeof(short));

		// Rotation
		ReadMemory((objData1ptr + objData1::RotX), &Angle, sizeof(int)*3);

		// Position
		ReadMemory((objData1ptr + objData1::PosX), &Position, sizeof(float)*3);

		// Character Object 2
		ReadMemory((objData2ptr + objData2::CharID1), &CharID[0],	sizeof(char)*2);

		if (characterID() == CharacterID::TailsChao || characterID() == CharacterID::EggmanDarkChao)
			ReadMemory((objData2ptr + objData2::MechHP), &MechHP, sizeof(float));
		else if ((characterID() == CharacterID::SonicAmy || characterID() == CharacterID::ShadowMetal) && (characterID2() == CharacterID2::Sonic || characterID2() == CharacterID2::Shadow))
			ReadMemory((objData2ptr + objSonic::SpindashTimer), &SpinTimer, sizeof(short));

		// Powerups and Upgrades
		ReadMemory((objData2ptr + objData2::Powerups), &Powerups,	sizeof(short));
		ReadMemory((objData2ptr + objData2::Upgrades), &Upgrades,	sizeof(int));

		// Speed
		ReadMemory((objData2ptr + objData2::HSpeed),	&Speed[0],	sizeof(float)*2);
		ReadMemory((objData2ptr + objData2::BaseSpeed),	&baseSpeed,	sizeof(int));

		// Animation
		ReadMemory((objData2ptr + objData2::AnimPlayback),	&AnimPlayback,	sizeof(char));
		ReadMemory((objData2ptr + objData2::NextAnim),		&Animation[0],	sizeof(short)*3);
	}
}

void PlayerObject::write(AbstractPlayer* recvr)
{
	pointerEval();

	if (exists)
	{
		// objData1
		WriteMemory((this->objData1ptr + objData1::Action),	&recvr->Action,		sizeof(char));
		WriteMemory((this->objData1ptr + objData1::Status),	&recvr->Status,		sizeof(short));
		WriteMemory((this->objData1ptr + objData1::RotX),	&recvr->Angle,		sizeof(int)*3);
		WriteMemory((this->objData1ptr + objData1::PosX),	&recvr->Position,	sizeof(float)*3);

		// objData2
		// Ignoring character ID for now (0 and 1)
		WriteMemory((this->objData2ptr + objData2::Powerups),	&recvr->Powerups,		sizeof(short));
		WriteMemory((this->objData2ptr + objData2::Upgrades),	&recvr->Upgrades,		sizeof(int));
		WriteMemory((this->objData2ptr + objData2::HSpeed),		&recvr->Speed[0],		sizeof(float)*2);
		WriteMemory((this->objData2ptr + objData2::BaseSpeed),	&recvr->baseSpeed,		sizeof(float));
		WriteMemory((this->objData2ptr + objData2::NextAnim),	&recvr->Animation[0],	sizeof(short));

		if (characterID() == CharacterID::TailsChao || characterID() == CharacterID::EggmanDarkChao)
			WriteMemory((this->objData2ptr + objData2::MechHP), &recvr->MechHP, sizeof(float));
		else if ((characterID() == CharacterID::SonicAmy || characterID() == CharacterID::ShadowMetal)/* && (characterID2() == CharacterID2::Sonic || characterID2() == CharacterID2::Shadow)*/)
			WriteMemory((this->objData2ptr + objSonic::SpindashTimer), &recvr->SpinTimer, sizeof(short));
	}
}

void PlayerObject::Teleport(AbstractPlayer* recvr)
{
	pointerEval();

	if (exists)
	{
		memcpy(&Position[0], &recvr->Position[0], (sizeof(float)* 3));
		Speed[0] = 0; Speed[1] = 0;

		// Pull the position from other player, nullify
		// momentum, and write.
		WriteMemory((objData1ptr + objData1::PosX),		&Position[0],	(sizeof(float)*3));
		WriteMemory((objData2ptr + objData2::HSpeed),	&Speed[0],		(sizeof(float)*2));
	}
}