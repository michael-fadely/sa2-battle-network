#include "stdafx.h"
#include "Globals.h"

void FrameSync()
{
	using namespace nethax;
	using namespace Globals;

	if (!isConnected())
		return;

	if (Networking->isServer())
	{
		// TODO: Implement Demand function that builds a packet on the spot and immediately dispatches it.
		Broker->Request(MessageID::S_FrameCount, true, true);
		Broker->Finalize();
		Broker->SendReady(MessageID::S_FrameCount);

		if (!Broker->WaitForPlayers(MessageID::S_FrameCount))
			return;
	}
	else
	{
		if (!Broker->WaitForPlayers(MessageID::S_FrameCount))
			return;
		Broker->SendReady(MessageID::S_FrameCount);
	}
}
