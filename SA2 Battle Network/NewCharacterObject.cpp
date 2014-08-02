#include "ModLoaderExtensions.h"

#include "NewPlayerObject.h"

PlayerObject::PlayerObject(ObjectMaster* player)
{
	LastPointer = nullptr;
	Initialize();
	Set(player);
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

void PlayerObject::Set(ObjectMaster* player)
{
	if (player != LastPointer)
	{
		LastPointer = player;
		Initialize();
	}

	if (player != nullptr)
	{
		Data1.Action = player->Data1->Action;
		Data1.Status = player->Data1->Status;
		Data1.Rotation = player->Data1->Rotation;
		Data1.Position = player->Data1->Position;
		Data1.Scale = player->Data1->Scale;

		Data2.CharID = player->Data2->CharID;
		Data2.CharID2 = player->Data2->CharID2;
		Data2.Powerups = player->Data2->Powerups;
		Data2.Upgrades = player->Data2->Upgrades;
		Data2.HSpeed = player->Data2->HSpeed;
		Data2.VSpeed = player->Data2->VSpeed;
		Data2.PhysData.BaseSpeed = player->Data2->PhysData.BaseSpeed;

		switch (player->Data2->CharID)
		{
		case Characters_Sonic:
		case Characters_Shadow:
		case Characters_Amy:
		case Characters_MetalSonic:
			//Sonic.HomingAttackTimer = ((SonicCharObj2*)player->Data2)->HomingAttackTimer;
			//Sonic.HomingRangeTimer = ((SonicCharObj2*)player->Data2)->HomingRangeTimer;
			Sonic.SpindashTimer = ((SonicCharObj2*)player->Data2)->SpindashTimer;
			break;

		case Characters_Tails:
			break;

		case Characters_Eggman:
			break;

		case Characters_Knuckles:
		case Characters_Rouge:
			break;

		case Characters_MechTails:
		case Characters_MechEggman:
			Data2.MechHP = player->Data2->MechHP;
			break;

		case Characters_SuperSonic:
		case Characters_SuperShadow:
			break;
		}
	}
}

/*
PlayerObject::PlayerObject(const ObjectMaster* player)
{
	data1 = nullptr;
	data2 = nullptr;
	Initialize(player);
}

PlayerObject::~PlayerObject()
{
	if (data1 != nullptr)
	{
		delete data1;
		data1 = nullptr;
	}

	if (data2 != nullptr)
		Deinitialize();
}

void PlayerObject::Deinitialize()
{
	switch (CharacterID)
	{
	default:
		delete (CharObj2*)data2;
		break;

	case Characters_Sonic:
	case Characters_Shadow:
	case Characters_Amy:
	case Characters_MetalSonic:
		delete (SonicCharObj2*)data2;
		break;

	case Characters_Tails:
		delete (TailsCharObj2*)data2;
		break;

	case Characters_Eggman:
		delete (EggmanCharObj2*)data2;
		break;

	case Characters_Knuckles:
	case Characters_Rouge:
		delete (KnucklesCharObj2*)data2;
		break;

	case Characters_MechTails:
	case Characters_MechEggman:
		delete (MechEggmanCharObj2*)data2;
		break;

	case Characters_SuperSonic:
	case Characters_SuperShadow:
		delete (SuperSonicCharObj2*)data2;
		break;
	}

	data2 = nullptr;
}

const bool PlayerObject::Initialize(const ObjectMaster* player)
{
	bool result = false;
	CharacterID = player->Data2->CharID;

	if (player != nullptr)
	{
		data1->Action = player->Data1->Action;
		data1->Status = player->Data1->Status;
		data1->Rotation = player->Data1->Rotation;
		data1->Position = player->Data1->Position;
		data1->Scale = player->Data1->Scale;

		if (data2 != nullptr)
			Deinitialize();

		switch (CharacterID)
		{
		default:
			data2 = new CharObj2;
			result = true;
			break;

		case Characters_Sonic:
		case Characters_Shadow:
		case Characters_Amy:
		case Characters_MetalSonic:
			data2 = new SonicCharObj2;
			result = true;
			break;

		case Characters_Tails:
			data2 = new TailsCharObj2;
			result = true;
			break;

		case Characters_Eggman:
			data2 = new EggmanCharObj2;
			result = true;
			break;

		case Characters_Knuckles:
		case Characters_Rouge:
			data2 = new KnucklesCharObj2;
			result = true;
			break;

		case Characters_MechTails:
		case Characters_MechEggman:
			data2 = new MechEggmanCharObj2;
			result = true;
			break;

		case Characters_SuperSonic:
		case Characters_SuperShadow:
			data2 = new SuperSonicCharObj2;
			result = true;
			break;
		}
	}
	return result;
}
*/