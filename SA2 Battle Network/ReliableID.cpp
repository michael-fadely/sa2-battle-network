#include <iostream>
#include <LazyTypedefs.h>

#include "Common.h"

#include "ReliableID.h"

using namespace std;

void reliableID::update(uint time)
{
	if (Duration(lastUpdate) >= time)
	{
		lastUpdate = millisecs();
		prune();
		return;
	}
	else
		return;
}

void reliableID::addID(uint id)
{
	rID add;
	add.id = 0;
	add.timeAdded = 0;

	add.id = id;
	add.timeAdded = millisecs();
	idList.push_front(add);
	cout << ">> Added received ID " << id << endl;
}

bool reliableID::checkID(uint id)
{
	if (!idList.empty())
	{
		for (auto i = idList.begin(); i != idList.end(); i++)
		{
			if (i->id == id)
			{
				i->timeAdded = millisecs();
				return true;
			}
		}
		return false;
	}

	else
		return false;
}

void reliableID::prune()
{
	if (!idList.empty())
	{
		for (auto i = idList.begin(); i != idList.end();)
		{
			if (Duration(i->timeAdded) >= 5000)
			{
				//cout << "Found old ID; pruning from that point on." << endl;
				i = idList.erase(i, idList.end());
				return;
			}
			else
				++i;
		}
	}
	else
		return;
}


