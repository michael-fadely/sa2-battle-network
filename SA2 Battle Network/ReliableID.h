#pragma once

#include <list>

struct rID
{
	unsigned int id;
	unsigned int timeAdded;
};

class reliableID
{
	private:
		std::list<rID> idList;

	public:
		void update(unsigned int time);
		void addID(unsigned int id);
		bool checkID(unsigned int id);
		void prune();

		unsigned int lastUpdate;
};