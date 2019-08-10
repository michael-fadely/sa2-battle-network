#include "stdafx.h"

#include <SA2ModLoader.h>
#include "globals.h"
#include "CommonEnums.h"

using namespace nethax;

extern "C" __declspec(dllexport) void OnFrame()
{
	if (!globals::is_initialized())
	{
		return;
	}

	auto this_thing = globals::networking->is_server() && CurrentMenu == Menu::battle && CurrentSubMenu > SubMenu2P::i_start;

	if (!globals::is_connected() || this_thing)
	{
		globals::program->connect();

		if (!this_thing)
		{
			return;
		}
	}

	if (!Program::can_connect())
	{
		globals::program->disconnect();
		return;
	}

	globals::broker->receive_loop();

	if (globals::broker->connection_timed_out())
	{
		PrintDebug("<> Connection timed out.");
		globals::program->disconnect();
	}

	globals::broker->send_system();
	globals::broker->send_player();
	globals::broker->send_menu();
}
