#include "stdafx.h"

#include <SA2ModLoader.h>
#include "globals.h"

using namespace nethax;

extern "C" __declspec(dllexport) void OnFrame()
{
	if (!globals::is_initialized())
	{
		return;
	}

	const bool can_connect = Program::can_connect();

	const bool server_can_accept_more = globals::broker->is_server() &&
	                                    can_connect;

	if (!globals::is_connected() || server_can_accept_more)
	{
		globals::program->connect();

		if (!server_can_accept_more)
		{
			return;
		}
	}

	if (!globals::is_connected() && !can_connect)
	{
		globals::program->disconnect();
		return;
	}

	globals::broker->receive_loop();

	if (globals::broker->connection_timed_out())
	{
		PrintDebug("<> Connection timed out.");
		globals::program->disconnect();
		return;
	}

	globals::broker->send_system();
	globals::broker->send_player();
	globals::broker->send_menu();
}
