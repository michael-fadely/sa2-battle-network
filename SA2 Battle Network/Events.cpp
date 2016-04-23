#include "stdafx.h"

#include "Events.h"

#include "PoseEffect2PStartMan.h"
#include "ItemBoxItems.h"
#include "EmeraldSync.h"
#include "CharacterSync.h"
#include "AddHP.h"
#include "AddRings.h"
#include "HurtPlayer.h"
#include "OnStageChange.h"
#include "Random.h"

void nethax::events::Initialize()
{
	InitCharacterSync();
	InitEmeraldSync();
	InitItemBoxItems();
	InitPoseEffect2PStartMan();
	InitAddHP();
	InitAddRings();
	InitHurtPlayer();
	InitOnStageChange();
	random::InitRandom();
}

void nethax::events::Deinitialize()
{
	DeinitCharacterSync();
	DeinitEmeraldSync();
	DeinitItemBoxItems();
	DeinitPoseEffect2PStartMan();
	DeinitAddHP();
	DeinitAddRings();
	DeinitHurtPlayer();
	DeinitOnStageChange();
	random::DeinitRandom();
}
