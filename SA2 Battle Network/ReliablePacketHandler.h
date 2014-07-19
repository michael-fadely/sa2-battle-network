// Only include once.
#pragma once

// Class declarations:
class QSocket;
class PacketHandler;
namespace Application { class Program; }

// Template-related includes:
#include <deque>

class reliablePacket
{
public:
	unsigned char* message;
	unsigned int msgLength;
	unsigned int uid;
	unsigned int lifespan;
	unsigned int timeAdded;
	
	reliablePacket(QSocket* Socket, unsigned int span);
	~reliablePacket();
};


class reliableQueue
{
	friend class Application::Program;
private:
	std::deque<reliablePacket*> deque;

	// Check the front of the reliable message deque (oldest) for the UID specified.
	inline bool checkFrontID(unsigned int uid)
	{
		return (deque.front()->uid == uid);
	}

	// Check the back of the reliable message deque (newest) for the UID specified.
	inline bool checkBackID(unsigned int uid)
	{
		return (deque.back()->uid == uid);
	}

public:
	// Add an entry to the reliable message deque
	bool add(QSocket* socket);

	// Delete an entry from the reliable message deque
	bool del(unsigned int uid);

	// Check if the deque is empty.
	inline bool isEmpty()
	{
		return deque.empty();
	}

	inline bool find(reliablePacket* input)
	{
		// Check for errors:
		if (input == nullptr) return false;
		if (isEmpty()) return false;

		// Local variable(s):
		bool response(false);

		for (auto i = deque.begin(); i != deque.end(); i++)
		{
			if (*i == input || (*i)->uid == input->uid)
			{
				response = true;
				break;
			}
		}

		return response;
	}

	inline bool find(unsigned int uid)
	{
		// Check for errors:
		if (isEmpty()) return false;

		// Local variable(s):
		bool response(false);

		for (auto i = deque.begin(); i != deque.end(); i++)
		{
			if ((*i)->uid == uid)
			{
				response = true;
				break;
			}
		}

		return response;
	}
	
	// Returns true if the packet has exceeded its lifespan
	bool isDead();

	// Writes the next message in the deque to the socket buffer
	void writeTopMsg(QSocket* socket);

	// Resend oldest not-yet-received reliable message in deque.
	unsigned int checkTimer;
	void resend(PacketHandler* networking);
};