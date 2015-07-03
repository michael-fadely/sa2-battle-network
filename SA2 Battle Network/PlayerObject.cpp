#include "ModLoaderExtensions.h"

#include "PlayerObject.h"

PlayerObject::PlayerObject(ObjectMaster* player)
{
	LastPointer = nullptr;
	Initialize();
	Copy(player);
}

PlayerObject::PlayerObject()
{
	LastPointer = nullptr;
	Initialize();
}

void PlayerObject::Initialize()
{
	Data1		= {};
	Data2		= {};
	Sonic		= {};
	Knuckles	= {};
	Eggman		= {};
	MechEggman	= {};
	Tails		= {};
	SuperSonic	= {};
}

void PlayerObject::Copy(ObjectMaster* source)
{
	if (source != LastPointer)
	{
		PrintDebug("Re-initializing local player object... [%08X != %08X]", LastPointer, source);
		LastPointer = source;
		Initialize();
	}

	if (source == nullptr)
		return;
	
	Data1.Action				= source->Data1->Action;
	Data1.Status				= source->Data1->Status;
	Data1.Rotation				= source->Data1->Rotation;
	Data1.Position				= source->Data1->Position;
	Data1.Scale					= source->Data1->Scale;

	Data2.CharID				= source->Data2->CharID;
	Data2.CharID2				= source->Data2->CharID2;
	Data2.Powerups				= source->Data2->Powerups;
	Data2.Upgrades				= source->Data2->Upgrades;
	Data2.HSpeed				= source->Data2->HSpeed;
	Data2.VSpeed				= source->Data2->VSpeed;
	Data2.PhysData.BaseSpeed	= source->Data2->PhysData.BaseSpeed;
	Data2.AnimInfo.Next			= source->Data2->AnimInfo.Next;

	switch (source->Data2->CharID2)
	{
	default:
		break;

	case Characters_Sonic:
	case Characters_Shadow:
	case Characters_Amy:
	case Characters_MetalSonic:
		Sonic.SpindashTimer = ((SonicCharObj2*)source->Data2)->SpindashTimer;
		break;

	case Characters_MechTails:
	case Characters_MechEggman:
		Data2.MechHP = source->Data2->MechHP;
		break;

		/*
	case Characters_Tails:
		break;

	case Characters_Eggman:
		break;

	case Characters_Knuckles:
	case Characters_Rouge:
		break;
			
	case Characters_SuperSonic:
	case Characters_SuperShadow:
		break;
		*/
	}
}

void PlayerObject::WritePlayer(ObjectMaster* destination, PlayerObject* source)
{
	if (source == nullptr)
		nullptr;
	
	destination->Data1->Action				= source->Data1.Action;
	destination->Data1->Status				= source->Data1.Status;
	destination->Data1->Rotation			= source->Data1.Rotation;
	destination->Data1->Position			= source->Data1.Position;
	destination->Data1->Scale				= source->Data1.Scale;

	destination->Data2->CharID				= source->Data2.CharID;
	destination->Data2->CharID2				= source->Data2.CharID2;
	destination->Data2->Powerups			= source->Data2.Powerups;
	destination->Data2->Upgrades			= source->Data2.Upgrades;
	destination->Data2->HSpeed				= source->Data2.HSpeed;
	destination->Data2->VSpeed				= source->Data2.VSpeed;
	destination->Data2->PhysData.BaseSpeed	= source->Data2.PhysData.BaseSpeed;
	destination->Data2->AnimInfo.Next		= source->Data2.AnimInfo.Next;

	switch (destination->Data2->CharID2)
	{
	default:
		break;

	case Characters_Sonic:
	case Characters_Shadow:
	case Characters_Amy:
	case Characters_MetalSonic:
		((SonicCharObj2*)destination->Data2)->SpindashTimer = source->Sonic.SpindashTimer;
		break;

	case Characters_MechTails:
	case Characters_MechEggman:
		destination->Data2->MechHP = source->Data2.MechHP;
		break;

		/*
	case Characters_Tails:
		break;

	case Characters_Eggman:
		break;

	case Characters_Knuckles:
	case Characters_Rouge:
		break;


	case Characters_SuperSonic:
	case Characters_SuperShadow:
		break;
		*/
	}
}

void PlayerObject::Teleport(ObjectMaster* target, PlayerObject* destination)
{
}
