#include "stdafx.h"
#include <SA2ModLoader.h>
#include "PoseEffect2PStartMan.h"
#include "globals.h"

static void __cdecl PoseEffect2PStartMan_Delete(ObjectMaster* obj)
{
	using namespace nethax;
	using namespace globals;

	if (is_connected())
	{
		broker->send_ready_and_wait(MessageID::S_RoundStart);
		MainCharObj1[0]->Action = 0;
		MainCharObj1[1]->Action = 0;
	}
}

static ObjectMaster* __stdcall AllocateObjectMaster_r(ObjectFuncPtr mainSub, int list, char* name)
{
	ObjectMaster* obj = AllocateObjectMaster(mainSub, list, name);

	if (obj != nullptr)
	{
		obj->DeleteSub = PoseEffect2PStartMan_Delete;
	}

	return obj;
}

static void __declspec(naked) AllocateObjectMasterBypass_asm()
{
	__asm
	{
		push [esp + 4]
		push esi
		push edi
		call AllocateObjectMaster_r
		retn
	}
}

void nethax::events::InitPoseEffect2PStartMan()
{
	WriteCall(reinterpret_cast<void*>(0x00477AAA), &AllocateObjectMasterBypass_asm);
}

void nethax::events::DeinitPoseEffect2PStartMan()
{
	WriteCall(reinterpret_cast<void*>(0x00477AAA), const_cast<void*>(AllocateObjectMasterPtr));
}
