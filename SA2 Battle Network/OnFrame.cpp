#include "stdafx.h"

#include <SA2ModLoader.h>
#include "Globals.h"
#include "CommonEnums.h"

using namespace nethax;

extern "C" __declspec(dllexport) void OnFrame()
{
	if (!Globals::isInitialized())
		return;

	auto thisthing = Globals::Networking->isServer() && CurrentMenu[0] == Menu::BATTLE && CurrentMenu[1] > SubMenu2P::I_START;
	if (!Globals::isConnected() || thisthing)
	{
		Globals::Program->Connect();
		if (!thisthing)
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
}
