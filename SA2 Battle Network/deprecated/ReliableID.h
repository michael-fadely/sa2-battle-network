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
		void update(const unsigned int time);
		void addID(const unsigned int id);
		const bool checkID(const unsigned int id);
		void prune();

		unsigned int lastUpdate;
};