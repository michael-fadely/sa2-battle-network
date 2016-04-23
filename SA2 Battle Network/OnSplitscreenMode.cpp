#include "stdafx.h"
#include <SA2ModLoader.h>
#include "AddressList.h"
#include "CommonEnums.h"
#include "OnSplitscreenMode.h"

using namespace nethax;

static bool toggle_safe = false;

static void IsSplitscreenSafe()
{
	toggle_safe = SplitscreenMode == 2;
}

// TODO: Make revertable
void events::InitOnSplitscreenMode()
{
	WriteJump((void*)0x004EB43B, IsSplitscreenSafe);
}

void events::ToggleSplitscreen()
{
	if (GameState == GameState::Ingame && TwoPlayerMode > 0 && toggle_safe)
	{
		int pressed = ControllerPointers[0]->PressedButtons & (Buttons_L | Buttons_R);
		int held = ControllerPointers[0]->HeldButtons & (Buttons_L | Buttons_R);
		if (pressed == (Buttons_L | Buttons_R) || pressed != 0 && held != 0 && pressed != held)
		{
			if (SplitscreenMode == 1)
				SplitscreenMode = 2;
			else if (SplitscreenMode == 2)
				SplitscreenMode = 1;
		}
	}
}
