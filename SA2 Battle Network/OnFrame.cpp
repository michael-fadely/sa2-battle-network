#include <SA2ModLoader.h>

#include "Globals.h"			// for Globals :specialed:

using namespace sa2bn;

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
		Globals::Program->Disconnect(false);
		return;
	}

	Globals::Broker->ReceiveLoop();

	if (Globals::Broker->ConnectionTimedOut())
	{
		PrintDebug("<> Connection timed out.");
		Globals::Program->Disconnect(true);
	}

	Globals::Broker->SendSystem();
	Globals::Broker->SendPlayer();
	Globals::Broker->SendMenu();
}
