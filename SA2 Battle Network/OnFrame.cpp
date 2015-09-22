#include "stdafx.h"

#include <SA2ModLoader.h>
#include "Globals.h"			// for Globals :specialed:

using namespace nethax;

extern "C" __declspec(dllexport) void OnFrame()
{
	if (!Globals::isInitialized())
		return;

	if (!Globals::isConnected())
	{
		Globals::Program->Connect();
		return;
	}

	if (!Globals::Program->CheckConnectOK())
	{
		Globals::Program->Disconnect();
		return;
	}

	Globals::Broker->ReceiveLoop();

	if (Globals::Broker->ConnectionTimedOut())
	{
		PrintDebug("<> Connection timed out.");
		Globals::Program->Disconnect();
	}

	Globals::Broker->SendSystem();
	Globals::Broker->SendPlayer();
	Globals::Broker->SendMenu();

	if (GameState == GameState::Ingame && TwoPlayerMode > 0)
	{
		if ((ControllersRaw[0].HeldButtons & Buttons_L && ControllersRaw[0].PressedButtons & Buttons_R) ||
			(ControllersRaw[0].PressedButtons & Buttons_L && ControllersRaw[0].HeldButtons & Buttons_R) ||
			(ControllersRaw[0].PressedButtons & Buttons_L && ControllersRaw[0].PressedButtons & Buttons_R))
		{
			if (SplitscreenMode == 1)
				SplitscreenMode = 2;
			else if (SplitscreenMode == 2)
				SplitscreenMode = 1;
		}
	}
}
