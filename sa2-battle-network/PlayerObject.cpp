#include "stdafx.h"

#include "ModLoaderExtensions.h"
#include "PlayerObject.h"

PlayerObject::PlayerObject(ObjectMaster* player)
{
	initialize();
	copy(player);
}

PlayerObject::PlayerObject()
{
	initialize();
}

void PlayerObject::initialize()
{
	data1       = {};
	data2       = {};
	sonic       = {};
	knuckles    = {};
	eggman      = {};
	mech_eggman = {};
	tails       = {};
	super_sonic = {};
}

void PlayerObject::copy(const ObjectMaster* source)
{
	if (source != last_object)
	{
		PrintDebug("Re-initializing local player object... [%08X != %08X]", last_object, source);
		last_object = source;
		initialize();
	}

	if (source == nullptr)
	{
		return;
	}

	data1.Action             = source->Data1->Action;
	data1.NextAction         = source->Data1->NextAction;
	data1.Status             = source->Data1->Status;
	data1.Rotation           = source->Data1->Rotation;
	data1.Position           = source->Data1->Position;
	data1.Scale              = source->Data1->Scale;
	data2.CharID             = source->Data2->CharID;
	data2.CharID2            = source->Data2->CharID2;
	data2.Powerups           = source->Data2->Powerups;
	data2.Upgrades           = source->Data2->Upgrades;
	data2.Speed              = source->Data2->Speed;
	data2.PhysData.BaseSpeed = source->Data2->PhysData.BaseSpeed;
	data2.AnimInfo.Next      = source->Data2->AnimInfo.Next;

	switch (source->Data2->CharID2)
	{
		default:
			break;

		case Characters_Sonic:
		case Characters_Shadow:
		case Characters_Amy:
		case Characters_MetalSonic:
			sonic.SpindashTimer = reinterpret_cast<SonicCharObj2*>(source->Data2)->SpindashTimer;
			break;

		case Characters_MechTails:
		case Characters_MechEggman:
			data2.MechHP = source->Data2->MechHP;
			break;
	}
}

void PlayerObject::write_player(ObjectMaster* destination, const PlayerObject* source)
{
	if (source == nullptr || destination == nullptr)
	{
		return;
	}

	destination->Data1->Status = source->data1.Status;

	// Use DoNextAction for Data1::Action if the DoNextAction status bit isn't present.
	if (source->data1.Status & Status_DoNextAction)
	{
		destination->Data1->Status |= Status_DoNextAction;
		destination->Data1->NextAction = source->data1.NextAction;
	}

	if (destination->Data1->Action != source->data1.Action)
	{
		destination->Data1->Action = source->data1.Action;
	}

	destination->Data1->Rotation = source->data1.Rotation;
	destination->Data1->Position = source->data1.Position;
	destination->Data1->Scale    = source->data1.Scale;

	destination->Data2->CharID             = source->data2.CharID;
	destination->Data2->CharID2            = source->data2.CharID2;
	destination->Data2->Powerups           = source->data2.Powerups;
	destination->Data2->Upgrades           = source->data2.Upgrades;
	destination->Data2->Speed              = source->data2.Speed;
	destination->Data2->PhysData.BaseSpeed = source->data2.PhysData.BaseSpeed;
	destination->Data2->AnimInfo.Next      = source->data2.AnimInfo.Next;

	switch (destination->Data2->CharID2)
	{
		default:
			break;

		case Characters_Sonic:
		case Characters_Shadow:
		case Characters_Amy:
		case Characters_MetalSonic:
			reinterpret_cast<SonicCharObj2*>(destination->Data2)->SpindashTimer = source->sonic.SpindashTimer;
			break;

		case Characters_MechTails:
		case Characters_MechEggman:
			destination->Data2->MechHP = source->data2.MechHP;
			break;
	}
}
