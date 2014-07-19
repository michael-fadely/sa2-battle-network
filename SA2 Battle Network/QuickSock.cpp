
/*
DISCLAIMER:

Copyright (c) 2014 - Anthony Diamond

Permission is hereby granted, free of qcharge, To any person obtaining a copy of this software And associated documentation files (the "Software"),
To deal in the Software without restriction, including without limitation the rights To use, copy, modify, merge, publish, distribute, sublicense,
And/Or sell copies of the Software, And To permit persons To whom the Software is furnished To do so, subject To the following conditions:

The above copyright notice And this permission notice shall be included in all copies Or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS Or IMPLIED, INCLUDING BUT Not LIMITED To THE WARRANTIES OF MERCHANTABILITY,
FITNESS For A PARTICULAR PURPOSE And NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS Or COPYRIGHT HOLDERS BE LIABLE For ANY CLAIM,
DAMAGES Or OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT Or OTHERWISE, ARISING FROM,
OUT OF Or IN CONNECTION WITH THE SOFTWARE Or THE USE Or OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef QSOCK_MONKEYMODE
	#include "QuickSock.h"
#endif

#ifndef WORDS_BIGENDIAN
	#if defined(QSOCK_WINDOWS_LEGACY) && QSOCK_WINDOWS_LEGACY == 1 || defined(CFG_GLFW_USE_MINGW) && CFG_GLFW_USE_MINGW == 1 && defined(QSOCK_MONKEYMODE)
		inline uqint htonf(const qfloat inFloat)
		{
			// Local variable(s):
			uqint retVal = 0;

			uqchar* floatToConvert = (uqchar*)&inFloat;
			uqchar* returnFloat = (uqchar*)&retVal;
		
			for (uqchar i = 0; i < sizeof(inFloat); i++)
				returnFloat[i] = floatToConvert[(sizeof(inFloat)-1)-i];
								
			return retVal;
		}
		
		inline qfloat ntohf(const uqint inFloat)
		{
			// Local variable(s):
			qfloat retVal = 0.0;

			uqchar* floatToConvert = (uqchar*) & inFloat;
			uqchar* returnFloat = (uqchar*) & retVal;
		
			for (uqchar i = 0; i < sizeof(inFloat); i++)
				returnFloat[i] = floatToConvert[(sizeof(inFloat)-1)-i];
								
			return retVal;
		}
		
		inline QSOCK_UINT64 htond(const double inFloat)
		{
			// Local variable(s):
			QSOCK_UINT64 retVal = 0;

			uqchar* floatToConvert = (uqchar*)&inFloat;
			uqchar* returnFloat = (uqchar*)&retVal;
		
			for (uqchar i = 0; i < sizeof(inFloat); i++)
				returnFloat[i] = floatToConvert[(sizeof(inFloat)-1)-i];
								
			return retVal;
		}
		
		inline qdouble ntohd(const QSOCK_UINT64 inFloat)
		{
			// Local variable(s):
			qdouble retVal = 0.0;

			uqchar* floatToConvert = (uqchar*)&inFloat;
			uqchar* returnFloat = (uqchar*)&retVal;
		
			for (uqchar i = 0; i < sizeof(inFloat); i++)
				returnFloat[i] = floatToConvert[(sizeof(inFloat)-1)-i];
								
			return retVal;
		}
	#endif
#endif

// Functions:
// Nothing so far.

// Classes:

// QSocket:

// Global variables:
#if defined(_WIN32) || defined(_WIN64)
	// An internal data-structure required for 'WinSock'. (Used for meta-data)
	WSADATA* QSocket::WSA_Data = NULL;
#endif

QSOCK_INT32 QSocket::BUFFERLEN = QSocket::DEFAULT_BUFFERLEN;
QSOCK_INT32 QSocket::socketAddress_Length = sizeof(socketAddress);
bool QSocket::socketsInitialized = false;
fd_set QSocket::fd;

// Constuctors & Destructors:

// This command sets up an object(Usually called by the constructor):
bool QSocket::setupObject(QSOCK_INT32 bufferLength)
{
	#if defined(QSOCK_AUTOINIT_SOCKETS) && QSOCK_AUTOINIT_SOCKETS == 1
		if (!initSockets()) return false;
	#endif

	// Initialize all of the pointers as 'NULL':
	inbuffer = NULL;
	result = NULL;
	boundAddress = NULL;
	outbuffer = NULL;

	_socket = INVALID_SOCKET;

	// Initialize the in and out buffers:
	if (bufferLength == 0) bufferLength = BUFFERLEN;
	_bufferlen = bufferLength;

	inbuffer = new uqchar[_bufferlen+1];
	inbuffer[_bufferlen] = '\0';

	outbuffer = new uqchar[_bufferlen+1];
	outbuffer[_bufferlen] = '\0';

	// Integer initialization:
	writeOffset = 0;
	readOffset = 0;
	outbufferlen = 0;
	inbufferlen = 0;
	port = (nativePort)0;

	// Initialize everything else we need to:

	// Initialize the timer-values.
	setTimeValues(false);

	// 'Zero-out' various fields:
	ZeroVar(hints);
	ZeroVar(si_Destination);
	ZeroVar(so_Destination);

	// By default, we'll assume the socket isn't for a server.
	_isServer = false;

	// This is more or less unused at the moment.
	manualDelete = false;

	// Initialize the socket's closed-state variable.
	socketClosed = true;

	// Initialize the socket's 'broadcast' variable.
	broadcastSupported = false;

	//outputPort = (nativePort)0;

	#if !defined(QSOCK_IPVABSTRACT)
		//outputIP = 0;
	#else
		//outputIP = "";
	#endif

	// Return the default response (true).
	return true;
}

// This command deletes some odds and ends of an object before the object is deleted(Usually called by the destructor):
bool QSocket::freeObject()
{
	// Namespace(s):
	using namespace std;

	// Check to see if the socket has been closed, if not, close it.
	if (!socketClosed) closeSocket();

	// Free the in and out buffers:

	// Delete the input buffer.
	delete [] inbuffer;
	
	// Delete the output buffer.
	delete [] outbuffer;

	// Return the default response (true).
	return true;
}

QSocket::QSocket(QSOCK_INT32 bufferlen)
{
	// Ensure we're not using Monkey:
	#ifndef QSOCK_MONKEYMODE
		if (socketsInitialized)
		{
			// Setup the object.
			setupObject(bufferlen);
		}
		else
		{
			// Exit the program.
			exit(EXIT_FAILURE);
		}
	#endif
}

QSocket::~QSocket()
{
	// If we're deleting it manually, free the object.
	if (manualDelete != true)
		freeObject();
}

// Functions:

bool QSocket::initSockets(QSOCK_INT32 bufferlen)
{
	// Namespace(s):
	using namespace std;

	// Output to the console.
	QSOCK_Print(endl << "QSocket -> initSockets()" << endl);
	QSOCK_Print("{");

	// Check if we've initialized yet, if we have, do nothing.
	if (socketsInitialized)
	{
		QSOCK_Print("	| Quick-Socket initialization failed: Already initialized |" << endl);
		QSOCK_Print("}");

		return false;
	} else {
		QSOCK_Print(endl << "	| QSOCK SESSION BEGIN |" << endl);
	}

	// Definitions:
	BUFFERLEN = bufferlen;
	
	#if defined(_WIN32) || defined(_WIN64)
		WSA_Data = new WSADATA();

		QSOCK_INT32 iResult = WSAStartup(MAKEWORD(2,2), WSA_Data);

		if (iResult != 0)
		{
			QSOCK_Print("	{WINDOWS}: WSAStartup() failed - Exact Error: " << iResult);

			// Cleanup everything related to WSA:

			// Delete the 'WSA_Data' variable.
			delete WSA_Data;

			WSACleanup();

			QSOCK_Print(endl << "	| Quick-Socket initialization failed |" << endl);
			QSOCK_Print("}");

			return false;
		}
		else
		{
			// Set the sockets as initialized.
			socketsInitialized = true;

			QSOCK_Print("	{WINDOWS}: WSAStartup() was successful.");

			QSOCK_Print("	{WINSOCK VERSION}: " << WSA_Data->wVersion << ", " << WSA_Data->wHighVersion << endl);
		}
	#endif

	// Initialize the 'File-descriptor set':
	QSOCK_Print("	Initializing 'File-descriptor set'...");
	FD_ZERO(&fd);
	QSOCK_Print("	File descriptors initialized.");

	// Complete the output process:
	QSOCK_Print(endl << "	| Quick-Sockets initialized |" << endl);
	QSOCK_Print("}");

	// Return the default response.
	return true;
}

bool QSocket::deinitSockets()
{
	// Namespace(s):
	using namespace std;

	QSOCK_Print("");
	QSOCK_Print("QSocket -> deinitSockets()" << endl);
	QSOCK_Print("{");

	if (!socketsInitialized)
	{
		QSOCK_Print("	| Quick-Socket deinitialization failed: Already deinitialized |");
		QSOCK_Print("}");

		return false;
	}

	// Free all of the objects we need to:
	#if defined(_WIN32) || defined(_WIN64)
		delete WSA_Data; WSA_Data = NULL;

		// Cleanup WinSock functionality.
		WSACleanup();

		// Output to the console.
		QSOCK_Print("	{WINDOWS}: WSACleanup() was successful.");
	#endif

	// Asign the socketsInitialized variable back to false.
	socketsInitialized = false;

	QSOCK_Print("");
	QSOCK_Print("	| Quick-Sockets deinitialized |");
	QSOCK_Print("");
	QSOCK_Print("	| QSOCK SESSION END |");
	QSOCK_Print("");
	QSOCK_Print("}");

	// Return something
	return true;
}

// Methods:
qint QSocket::bindInternalSocket(qint filter)
{
	// Namespace(s):
	using namespace std;

	// Local variable(s):
	qint bindResult = 0;

	// Used for socket options:
	QSOCK_INT32 bAllow = 1;
	QSOCK_INT32 bDisable = 0;

	#if !defined(QSOCK_IPVABSTRACT)
		_socket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
		bindResult = bind(_socket, (const sockaddr*)boundAddress, socketAddress_Length);
	#else
		// Bind the socket using the one of the addresses in 'result':
		for (boundAddress = result; boundAddress != NULL; boundAddress = boundAddress->ai_next)
		{
			if (filter != -1 && boundAddress->ai_family != filter)
				continue;

			// Try to create a socket.
			_socket = socket(boundAddress->ai_family, boundAddress->ai_socktype, boundAddress->ai_protocol);

			// Check for errors while attempting socket creation:
			if (_socket == INVALID_SOCKET)
			{
				// Unable to create a socket, skip this attempt.
				continue;
			}

			// Assgin various socket options:
			#if defined(QSOCK_IPVABSTRACT)
				if
				(
					#if defined(QSOCK_IPVABSTRACT)
						boundAddress->ai_family == AF_INET6
					#else
						boundAddress->sin_family == AF_INET6
					#endif
				)
				{
					// Add support for IPV4:
					QSOCK_Print("		{?}: Attempting to initialize IPV4-compatibility support...");

					if (setsockopt(_socket, IPPROTO_IPV6, IPV6_V6ONLY, (const QSOCK_CHAR*)&bDisable, sizeof(bDisable)) != 0)
					{
						QSOCK_Print("		{?}: Unable to apply socket option: IPV6_ONLY");
						QSOCK_Print("		{-}: Continuing with hosting process anyway.");
        
						//return false;
					}
					else
					{
						QSOCK_Print("		{?}: IPV4-compatibility initialization complete: No errors found.");
					}
				}
			#endif

			if (isServer())
			{
				// Add support for broadcasting messages:
				QSOCK_Print("		{?}: Attempting to initialize broadcast support...");
			
				if
				(
					#if defined(QSOCK_IPVABSTRACT)
						boundAddress->ai_family != AF_INET
					#else
						boundAddress->sin_family != AF_INET
					#endif
					|| setsockopt(_socket, SOL_SOCKET, SO_BROADCAST, (const QSOCK_CHAR*)&bAllow, sizeof(bAllow)) < 0
				)
				{
					QSOCK_Print("		{?}: Unable to apply socket option: SO_BROADCAST");

					broadcastSupported = false;

					QSOCK_Print("		{!}: Continuing with hosting process anyway (You may need to look into this).");
        
					//return false;
				}
				else
				{
					broadcastSupported = true;

					QSOCK_Print("		{?}: Broadcast initialization complete: No errors found.");
				}

				// End the socket functionality section.
				QSOCK_Print("	}" << endl);
			}

			// Attempt to bind the socket.
			bindResult = bind(_socket, boundAddress->ai_addr, boundAddress->ai_addrlen);

			// Check if the bind attempt was successful.
			if (bindResult == 0)
			{
				// The socket was successfully bound, exit the loop.
				break;
			}
			else
			{
				// Close the socket.
				shutdownInternalSocket();
			}
		}
	#endif

	// Return the error code produced from 'bind'.
	return bindResult;
}

bool QSocket::bindSocket(const nativePort internalPort)
{
	// Namespace(s):
	using namespace std;

	// Setup the socket's information:
	#if !defined(QSOCK_IPVABSTRACT)
		hints.ai_family = AF_INET;
	#else
		//hints.ai_family = AF_INET; // This will be changed to 'AF_UNSPEC' later on, assuming an IPV4 address couldn't be found.
		hints.ai_family = AF_UNSPEC;
	#endif

	// Set the information for 'hints':
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	// Setup the 'result' socket 'address_in' with 'hints', the internal port, and 'INADDR_ANY' as the address:
	#if !defined(QSOCK_IPVABSTRACT)
		result = new socketAddress(); // Also known as 'sockaddr_in'

		#if defined(_WIN32) || defined(_WIN64)
			result->sin_addr.S_un.S_addr =
		#else
			result->sin_addr.s_addr =
		#endif

		htonl(INADDR_ANY);

		result->sin_family = hints.ai_family;
		result->sin_port = htons(internalPort);
	#else
		{
			// Convert the port to an 'std::string'.
			stringstream portSStream; portSStream << internalPort;

			/*
			QSOCK_CHAR* portStr = new QSOCK_CHAR[portSStream.str().length()+1];
			memcpy(portStr, portSStream.str().c_str(), portSStream.str().length());
			portStr[portSStream.str().length()] = '\0';
			*/
			
			/*
			// Check for IPV4 addresses first:
			if (getaddrinfo(NULL, (const QSOCK_CHAR*)portStr, &hints, &result) != 0) // portSStream.str().c_str()
			{
				// We weren't able to find an IPV4 address, clean things up, and try with any IP-version:
				if (result != NULL) freeaddrinfo(result); result = NULL; // delete [] result;
				
				// Assign the ip-family to unspecific.
				hints.ai_family = AF_INET;
			*/

			if (getaddrinfo(NULL, (const QSOCK_CHAR*)portSStream.str().c_str(), &hints, &result) != 0) // portStr
			{
				// Delete the address-info result.
				if (result != NULL) freeaddrinfo(result); //delete [] result;

				#if defined(_WIN32) || defined(_WIN64)
					#if defined(QSOCK_COUT)
						#if QSOCK_COUT == 1
							cout << "	{WINDOWS}: Internal socket initialization failed - WSAGetLastError(): " << WSAGetLastError() << endl;
						#endif
					#endif
				#else
					#if defined(QSOCK_COUT)
						#if QSOCK_COUT == 1
							cout << "	Internal socket initialization failed. - Error # " << errno << endl;
						#endif
					#endif
				#endif

				// Since we can't continue, tell the user.
				return false;
			}

			//delete [] portStr;
			//}

			hints.ai_flags = 0;
		}
	#endif

	// Create the socket using 'hints':
	{
		// Local variable(s):

		// The result-variable used for the bind-result.
		qint bindResult = 0;

		QSOCK_Print("");
		QSOCK_Print("	Initializing socket functionality" << endl);
		QSOCK_Print("	{");

		#if !defined(QSOCK_IPVABSTRACT)
			// Assign the bound-address variable to 'result'.
			boundAddress = result;

			// Bind the socket, using the address 'boundAddress' is pointing to.
			bindResult = bindInternalSocket();
		#else
			// Try IPV6 before attempting to use any protocol.
			bindResult = bindInternalSocket(AF_INET6);

			// We weren't able to use IPV6, try any protocol:
			if (bindResult != 0)
			{
				bindResult = bindInternalSocket();
			}
		#endif

		// Check for any errors while creating the socket:
		if (_socket == INVALID_SOCKET)
		{
			#if defined(_WIN32) || defined(_WIN64)
				QSOCK_Print("	{WINDOWS}: Internal socket initialization failed - WSAGetLastError(): " << WSAGetLastError());
			#else
				QSOCK_Print("	Internal socket initialization failed. - Error # " << errno);
			#endif

			QSOCK_Print("}");

			closeSocket();

			return false;
		}

		// Check if our bind-attempt was successful:
		if (bindResult == SOCKET_ERROR)
		{
			#if defined(_WIN32) || defined(_WIN64)
				QSOCK_Print("	{WINDOWS}: Unable to bind the internal socket - WSAGetLastError(): " << WSAGetLastError());
			#else
				QSOCK_Print("	Unable to bind the internal socket. - Error # " << errno);
			#endif
		
			QSOCK_Print("}");
			
			// Close the internal socket.
			closeSocket();

			// Return a negative response.
			return false;
		}

		///* Do not set the family of so_Destination first, this will screw up checks made later on:
		// Assign the IO addresses' families:
		#if !defined(QSOCK_IPVABSTRACT)
			si_Destination.sin_family = boundAddress->sin_family;
			//so_Destination.sin_family = si_Destination.sin_family;
		#else
			si_Destination.sa_family = boundAddress->ai_addr->sa_family; //boundAddress->ai_family;
			//so_Destination.sa_family = si_Destination.sa_family;
		#endif
		//*/
	}

	// Return the default response.
	return true;
}

#if !defined(QSOCK_MONKEYMODE)
bool QSocket::connect(nativeString address, const nativePort externalPort, const nativePort internalPort)
{
#else
bool QSocket::connect(nativeString _address, const QSOCK_INT32 _externalPort, const QSOCK_INT32 _internalPort)
{
	// Locsl variable(s):
	const nativePort externalPort = (nativePort)_externalPort;
	const nativePort internalPort = (nativePort)_internalPort;
#endif
	// Namespace(s):
	using namespace std;

	if (!socketClosed) return false;

	// If we're using Monkey, convert 'address' from a Monkey-based string to a normal 'nativeString':
	#if defined(QSOCK_MONKEYMODE)
		// Convert the monkey-based string into a standard-string:
		std::string address(_address.Length()+1, '\0');
		
		// Convert the Monkey-native address-string to a char array.
		for (uqint index = 0; index < _address.Length(); index++)
			address[index] = _address[index];
	#endif

	// Check if we're using IPV6 or not:
	/*
	#if defined(QSOCK_IPV6)
		for (QSOCK_UINT32_LONG ipIndex = 0; ipIndex < (QSOCK_UINT32_LONG)strlen(address); ipIndex++)
		{
			if (address[ipIndex] == ':')
			{
				Manual_IPV6Enabled = true;
				break;
			}
		}
	#endif
	*/

	// Tell the user we're connecting.
	QSOCK_Print(endl << "QSocket -> connect()" << endl << "{");
	
	// Definitions:
	
	// Set the port variable.
	this->port = externalPort;
	
	// Set the 'isServer' variable.
	this->_isServer = false;

	// Bind and initialize the internal socket.
	if (!bindSocket(internalPort)) return false;

	// Setup our destination using the external port, and the address:
	if (!setupDestination(address, this->port)) return false;

	// Output to the console:
	#if defined(QSOCK_COUT) && QSOCK_COUT == 1
		cout << "	Client connection established(" << externalPort << "E, " << internalPort << "I)" << endl;
	#endif

	QSOCK_Print("");
	QSOCK_Print("	Finishing up a few things...");

	// Set the timer values.
	setTimeValues(true);

	// Finish doing any needed operations:

	// Set the socket as 'open'.
	this->socketClosed = false;

	// Finish up the message from before.
	QSOCK_Print("	Operations complete.");

	// Print the ending bracket to the console.
	QSOCK_Print("}");

	// Return the default response.
	return true;
}

#if defined(QSOCK_MONKEYMODE)
bool QSocket::host(const QSOCK_INT32 _externalPort)
{
	nativePort externalPort = (nativePort)(_externalPort);
#else
bool QSocket::host(nativePort externalPort)
{
#endif
	// Namespace(s):
	using namespace std;

	// Tell the user we're hosting.
	QSOCK_Print(endl << "QSocket -> host()" << endl << "{");

	// Definitions:
	
	// Set the port variable.
	this->port = externalPort;

	// Set the isServer variable:
	this->_isServer = true;

	// Bind and initialize the internal socket.
	if (!bindSocket(externalPort)) return false;

	// Print some extra things to the console:
	#ifdef QSOCK_COUT
		#if QSOCK_COUT == 1
			cout << "	Server connection established(" << externalPort << ")." << endl;
		#endif
	#endif

	QSOCK_Print("");
	QSOCK_Print("	Finishing up a few things...");

	// Set the timer values.
	setTimeValues(true);

	// Finish doing any needed operations:
	this->socketClosed = false;

	// Finish up the message from before.
	QSOCK_Print("	Operations complete.");

	// Print the ending bracket to the console.
	QSOCK_Print("}");

	// Return with a value of 1(true).
	return true;
}

bool QSocket::setupDestinationV4(QSOCK_UINT32_LONG address, nativePort externalPort)
{
	#if !defined(QSOCK_IPVABSTRACT)
		if (externalPort == (nativePort)0)
		{
			externalPort = msgPort();

			// If we still don't have a port to work with, tell the user:
			if (externalPort == (nativePort)0)
				return false;
		}

		// If the address is set to zero, and we can't broadcast, try the IP of the last message received:
		if (address == 0 && !broadcastSupported)
		{
			// Get the IP from the last packet received.
			address = msgIP();

			// If we still don't have an IP to work with, tell the user:
			if (address == 0)
				return false;
		}
		
		// Assign the output-family to IPV4.
		so_Destination.sin_family = AF_INET;

		// Assign the output-port to 'externalPort'.
		so_Destination.sin_port = ntohs((uqshort)externalPort);

		// Assign the output-address to 'address':
		#if defined(_WIN32) || defined(_WIN64)
			so_Destination.sin_addr.S_un.S_addr =
		#else
			so_Destination.sin_addr.s_addr =
		#endif

		htonl(address);

		// No errors were found, return as usual.
		return true;
	#else
		// Call the main 'QSOCK_IPVABSTRACT' compliant version of 'setupDestination'.
		return setupDestination(IntToStringIP(address), externalPort);
	#endif

	// This point should never be reached.
	return false;
}

bool QSocket::setupDestination(std::string address, nativePort externalPort)
{
	// Namespace(s):
	using namespace std;

	// Local variable(s):
	bool response(false);

	#if !defined(QSOCK_IPVABSTRACT)
		// Call the IPV4 version of 'setupDestination'.
		response = setupDestination(StringToIntIP(address), externalPort);
	#else
		// If I end up adding to this function later, I'll want what's in here to be cleaned up first:
		{
			if (address.length() == 0)
			{
				address = (std::string)msgIP();

				if (address.length() == 0)
				{
					// We were unable to find a suitable address.
					return false;
				}
			}

			if (externalPort == (nativePort)0)
			{
				externalPort = msgPort();

				// Check if we've been given a valid port, and if not, return false:
				if (externalPort == (nativePort)0)
					return false;
			}

			// Local variable(s):
			addrinfo* so_Destination_result = NULL;
			stringstream portSStream; portSStream << externalPort;
			
			// inet_pton(hints.sa_family, address, &so_Destination)

			if (getaddrinfo(address.c_str(), portSStream.str().c_str(), &hints, &so_Destination_result) != 0)
			{
				cout << QSOCK_ERROR << endl;

				// Clean up the address-list.
				freeaddrinfo(so_Destination_result);

				// Since we can't continue, tell the user.
				return false;
			}

			// Copy the result from 'getaddrinfo' to 'so_Destination', then delete the old copy of the result:
			memcpy(&so_Destination, so_Destination_result->ai_addr, sizeof(so_Destination));

			// Clean up the address-list.
			freeaddrinfo(so_Destination_result);

			// Set the response to 'true'.
			response = true;
		}
	#endif

	// Return the default response.
	return response;
}

// Update related:

// This is our generic update command(It applies to both the client and server).
QSOCK_INT32 QSocket::listenSocket()
{
	// General update code:

	// Free the message buffers:
	clearInBuffer();
	clearOutBuffer();

	if (isServer())
	{
		// Run the server's update routine.
		return hostUpdate();
	}

	// Run the client's update routine.
	return clientUpdate();
}

// 'clientUpdate' & 'hostUpdate' have been moved to the main header.

// These still hold all of their code, they're just private now:
QSOCK_INT32 QSocket::hostUpdate()
{
	// Namespace(s):
	using namespace std;

	// The main program:

	// Check for messages:
	readMsg();

	// Return the default response
	return 0;
}

QSOCK_INT32 QSocket::clientUpdate()
{
	// Namespace(s):
	using namespace std;

	// The main program:

	// Check for messages:
	readMsg();

	// Return the default response
	return 0;
}

// Input related:
QSOCK_INT32 QSocket::readAvail()
{
	// Namespace(s):
	using namespace std;

	// Definitions:
	QSOCK_INT32 response = 0;

	if (!FD_ISSET(_socket, &fd))
	{
		inbufferlen = 0;
		readOffset = 0;
		// Nothing so far.
	}
	else
	{		
		response = (inbufferlen = recvfrom(_socket, (QSOCK_CHAR*)inbuffer, _bufferlen, 0, (sockaddr*)&si_Destination, &socketAddress_Length));


		// Check for errors:
		if (response == SOCKET_ERROR && QSOCK_ERROR != WSAENETRESET && QSOCK_ERROR != WSAECONNRESET)
		{
			#if defined(QSOCK_COUT_DETAILED) && QSOCK_COUT_DETAILED == 1
				QSOCK_CSTREAM("	{?}");
				
				#if defined(_WIN32) || defined(_WIN64)
					QSOCK_CSTREAM(" {WINDOWS}");
				#endif

				QSOCK_CSTREAM(": QSocket -> readAvail(): (FD_ISSET) -> recvfrom() failed ");

				#if defined(_WIN32) || defined(_WIN64)
					QSOCK_CSTREAM(" - WSAGetLastError(): ");
				#else
					QSOCK_CSTREAM(" - Error # ");
				#endif

				QSOCK_CSTREAM(QSOCK_ERROR << endl);
			#endif

			response = SOCKET_ERROR;
		}
		else if (response > 0)
		{
			// Reset the length and offset for the input-buffer:
			readOffset = 0;

			// Assign 'so_Destination' to 'si_Destination'.
			memcpy(&so_Destination, &si_Destination, socketAddress_Length);
		}
	}

	// Check for messages:
	FD_ZERO(&fd);
	FD_SET(_socket, &fd);
	
	if (select(1, &fd, NULL, NULL, (timeval*)&tv) == SOCKET_ERROR)
	{
		// Commented for now, enable this if you need it:
		/*
		QSOCK_CSTREAM("{?}");
				
		#if defined(_WIN32) || defined(_WIN64)
			QSOCK_CSTREAM(" {WINDOWS}");
		#endif

		QSOCK_CSTREAM(": QSocket -> readAvail(): select() failed unexpectedly");

		#if defined(_WIN32) || defined(_WIN64)
			QSOCK_CSTREAM(" - WSAGetLastError(): " << WSAGetLastError() << endl);
		#else
			QSOCK_CSTREAM(" - Error # " << errno << endl);
		#endif
		*/
	}
	
	// Return the calculated response.
	return response;
}

QSOCK_INT32 QSocket::readMsg()
{
	// Poll for an available packet.
	QSOCK_INT32 response = readAvail();
	
	// Nothing extra so far.

	// Return the calculated response.
	return response;
}

bool QSocket::clearInBuffer()
{
	// 'Zero-out' the inbound-message buffer.
	ZeroMemory(inbuffer, _bufferlen);

	// Set the read-offset and inbound-message length to zero.
	readOffset = 0;
	inbufferlen = 0;

	// Return the default response (true).
	return true;
}

void QSocket::setTimeValues(bool init)
{
	// Set the time values:
	if (init)
	{
		FD_SET(_socket, &fd);

		tv.tv_usec = 0;
		tv.tv_sec = 0;

		// DO NOT UNCOMMENT THIS.
		select(1, &fd, NULL, NULL, &tv);
	}

	tv.tv_sec = TIMEOUT_SEC;
	tv.tv_usec = TIMEOUT_USEC;

	return;
}

nativeString QSocket::strMsgIP() const
{
	#if !defined(QSOCK_IPVABSTRACT)
		return QSocket::IntToStringIP(msgIP());
	#else
		// Namespace(s):
		using namespace std;
		
		// Local variable(s):
		//size_t outputLength(0);
		//void* dataPosition = NULL;

		// Allocate the needed c-string(s).
		QSOCK_CHAR output_cstr[NI_MAXHOST];

		// Allocate the output 'nativeString'.
		nativeString output;

		// Convert the address to a string:
		//inet_ntop(si_Destination.sa_family, dataPosition, output_cstr, outputLength); // (void*)&si_Destination.sa_data

		if (getnameinfo(&so_Destination, sizeof(sockaddr_in6), output_cstr, sizeof(output_cstr), NULL, 0, 0) != 0)
		{
			// Nothing so far.
		}

		// Convert the c-string to a 'nativeString':
		output = (nativeString)output_cstr;

		// Return the newly generated 'nativeString'.
		return output;
	#endif
}

QSOCK_UINT32_LONG QSocket::intMsgIP() const
{
	return

	#ifdef QSOCK_MONKEYMODE
		(QSOCK_INT32)
	#endif

	#if !defined(QSOCK_IPVABSTRACT)
		ntohl
		(
		#if defined(_WIN32) || defined(_WIN64)
			si_Destination.sin_addr.S_un.S_addr
		#else
			si_Destination.sin_addr.s_addr
		#endif
		);
	#else
		(si_Destination.sa_family == AF_INET) ? StringToIntIP(strMsgIP()) : 0;
	#endif
}

nativePort QSocket::msgPort() const
{
	// Check the destination's family, and based on that, return the internal port.
	#if !defined(QSOCK_IPVABSTRACT)
		/*
		switch (si_Destination.sin_family)
		{
			default: // case AF_INET:
		*/
				return (nativePort)ntohs(((sockaddr_in*)&si_Destination)->sin_port);
		/*		break;
		}*/
	#else
		// Allocate the needed c-string(s).
		QSOCK_CHAR serverInfo[NI_MAXSERV];

		// Convert the service name to a 'nativePort'
		if (getnameinfo((const socketAddress*)&si_Destination, socketAddress_Length, NULL, NULL, serverInfo, NI_MAXSERV, NI_DGRAM|NI_NUMERICSERV) != 0)
		{
			return (nativePort)0;
		}

		return (nativePort)atol(serverInfo);
	#endif

	// If we couldn't calculate the port, return zero.
	return (nativePort)0;
}

// Buffer reading related(Most are compressed due to their size):

// Some of the world's messiest code:

// Integer types:
qshort QSocket::readByte() { return (qshort)readOct(); } // The return type is 'qshort' due to streams. For characters, use readChar().
qchar QSocket::readChar() { return ((qchar)readOct()); } // This is command and readOct are recommended over readByte.
uqchar QSocket::readOct() { if ((QSOCK_INT32)(readOffset+sizeof(uqchar)) <= inbufferlen) { uqchar data(0); data = inbuffer[readOffset]; readOffset += sizeof(data); return data; } else { return 0; } }

qshort QSocket::readShort() { qshort data(0); if ((QSOCK_INT32)(readOffset+sizeof(data)) <= inbufferlen) { memcpy(&data, inbuffer+readOffset, sizeof(data)); readOffset += sizeof(data); if (isServer()) return ntohs(data); else return htons(data); } else { return SOCKET_ERROR; } }
QSOCK_INT32 QSocket::readInt() { QSOCK_INT32 data(0); if ((QSOCK_INT32)(readOffset+sizeof(data)) <= inbufferlen) { memcpy(&data, inbuffer+readOffset, sizeof(data)); readOffset += sizeof(data); if (isServer()) return ntohl(data); else return htonl(data); } else { return SOCKET_ERROR; } }
QSOCK_INT64 QSocket::readLong() { QSOCK_INT64 data(0); if ((QSOCK_INT32)(readOffset+sizeof(data)) <= inbufferlen) { memcpy(&data, inbuffer+readOffset, sizeof(data)); readOffset += sizeof(data); if (isServer()) return ntohll(data); else return htonll(data); } else { return SOCKET_ERROR; } }

uqchar* QSocket::UreadBytes(uqint count)
{
	// Definitions:
	if (count == 0)
		count = inbufferlen-readOffset;

	// Create a new unsigned 'qchar' array for output.
	uqchar* data = new uqchar[count+1];
	data[count] = '\0';

	if ((uqint)(readOffset+count) <= (uqint)inbufferlen)
	{
		//memcpy(data, inbuffer+readOffset, strlen((const qchar*)data));

		for (uqint index = 0; index <= count; index++)
		{
			data[index] = readOct();
		}
	} else {
		delete[] data;
		// Return nothing.
		return NULL;
	}
	
	// Return the information.
	return data;
}

qchar* QSocket::readBytes(uqint count) { return (qchar*)UreadBytes(count); }

// Floating-point types:
qfloat QSocket::readFloat() { QSOCK_UINT32 data(0); if ((QSOCK_INT32)(readOffset+sizeof(data)) <= inbufferlen) { memcpy(&data, inbuffer+readOffset, sizeof(data)); readOffset += sizeof(data); return ntohf(data); } else { return SOCKET_ERROR; } }
double QSocket::readDouble() { QSOCK_UINT64 data(0); if ((QSOCK_INT32)(readOffset+sizeof(data)) <= inbufferlen) { memcpy(&data, inbuffer+readOffset, sizeof(data)); readOffset += sizeof(data); return ntohd(data); } else { return SOCKET_ERROR; } }

// Other types:

// Line related:
nativeString QSocket::readLine()
{
	// Namespace(s):
	using namespace std;

	// Definitions:
	QSOCK_INT32 count = msgLength()-readOffset;

	if (count <= 0)
		return nativeString();

	QSOCK_UINT32 strLen = 0;
	uqchar* bytePosition = NULL;

	for (uqint index = readOffset; index < (uqint)inbufferlen; index++)
	{
		bytePosition = inbuffer+index;
		
		// Search for the correct bytes to end the line:
		if (*bytePosition == (uqchar)'\r' && (index+1) <= (uqint)inbufferlen)
		{
			// If we've found the final byte, break.
			if (*(bytePosition++) == (uqchar)'\n')
				break;
		}

		// Add to the final string-length.
		strLen++;
	}

	// Allocate the output-string.
	nativeString output;

	#if !defined(QSOCK_MONKEYMODE)
		// Copy part of the contents of 'inbuffer' to our output 'string'.
		output.assign((QSOCK_CHAR*)(inbuffer+readOffset), strLen);

		// Add to the offset.
		readOffset += strLen;
	#else
		// Allocate a temporary buffer.
		QSOCK_CHAR* tempBuffer = new QSOCK_CHAR[strLen+1]; tempBuffer[strLen] = '\0';

		// Copy part of the contents of 'inbuffer' to the temp-buffer.
		memcpy(tempBuffer, inbuffer+readOffset, strLen);

		// Allocate a 'string' for the output using 'tempBuffer'.
		output = tempBufer;

		// Delete the temporary buffer.
		delete [] tempBuffer;
	#endif

	// Return the output.
	return output;
}

// Output related:
QSOCK_INT32 QSocket::broadcastMsg(nativePort port) // Only really useful for servers/hosts.
{
	// Namespace(s):
	using namespace std;

	if (!isServer() || !broadcastSupported)
		return sendMsg(msgIP(), port);

	if (isServer() && broadcastSupported)
	{
		// Set the IPV4-destination.
		if (!setupDestination(INADDR_BROADCAST, port)) return SOCKET_ERROR;

		// Output the message.
		outputMessage();
	}

	return 0;
}

#if defined(QSOCK_MONKEYMODE)
QSOCK_INT32 QSocket::sendMsg(QSOCK_INT32 _IP, QSOCK_INT32 _port)
{
	QSOCK_UINT32_LONG IP = (QSOCK_UINT32_LONG)(_IP);
	nativePort port = (nativePort)(_port);
#else
QSOCK_INT32 QSocket::sendMsg(QSOCK_UINT32_LONG IP, nativePort port)
{
#endif
	// Namespace(s):
	using namespace std;

	// Setup the output-address structure with the needed information.
	if (!setupDestination(IP, port)) return SOCKET_ERROR;

	// Output the message.
	return outputMessage();
}


#if defined(QSOCK_MONKEYMODE)
QSOCK_INT32 QSocket::sendMsg(nativeString _strIP, QSOCK_INT32 _port)
{
	nativePort port = (nativePort)(_port);
	std::string strIP(_strIP.Length(), '\0');

	for (uqint index = 0; index < _strIP.Length(); index++)
	{
		strIP[index] = _strIP[index];
	}
#else
QSOCK_INT32 QSocket::sendMsg(nativeString strIP, nativePort port)
{
#endif

	#if !defined(QSOCK_IPVABSTRACT)
		return sendMsg(StringToIntIP(strIP), port);
	#else
		if (!setupDestination(strIP, port)) return SOCKET_ERROR;

		// Output the message.
		return outputMessage();
	#endif

	// Return the default response.
	return 0;
}

QSOCK_INT32 QSocket::sendMsg()
{
	if
	(
		#if !defined(QSOCK_IPVABSTRACT)
			so_Destination.sin_family == 0
		#else
			so_Destination.sa_family == 0
		#endif
	)
	{
	if (!setupDestination(msgIP(), msgPort()))
	{
		return SOCKET_ERROR;
	}
	}

	return outputMessage();
}

QSOCK_INT32 QSocket::outputMessage()
{
	// Namespace(s):
	using namespace std;

	// Ensure we have an address to work with.
	if
	(
		#if !defined(QSOCK_IPVABSTRACT)
			so_Destination.sin_family == 0
		#else
			so_Destination.sa_family == 0
		#endif
	)
	{
		return SOCKET_ERROR;
	}

	// Write to the console:
	#if defined(QSOCK_COUT_DETAILED) && QSOCK_COUT_DETAILED == 1
		QSOCK_Print("");
		QSOCK_Print("QSocket -> sendMsg()" << endl << "{");
	#endif

	// Send the current 'outbuffer' to the IP address specified, on the port specified:
	if (sendto(_socket, (QSOCK_CHAR*)outbuffer, outbufferlen, 0, (const sockaddr*)&so_Destination, socketAddress_Length) == SOCKET_ERROR) // No flags added for now.
	{
		#if defined(QSOCK_COUT) && defined(QSOCK_COUT_DETAILED) && QSOCK_COUT_DETAILED == 1
			#if QSOCK_COUT == 1
				if
				(
					#if defined(_WIN32) || defined(_WIN64)
						WSAGetLastError()
					#else
						errno
					#endif

					!= WSAECONNRESET
				)
				{
					cout << "	{?}";
					
					#if defined(_WIN32) || defined(_WIN64)
						cout << " {WINDOWS}";
					#endif

					cout << ": QSocket -> sendMsg(): sendto(): failed";

					#if defined(_WIN32) || defined(_WIN64)
						cout << " - WSAGetLastError(): " << WSAGetLastError() << endl;
					#else
						cout << " - Error # " << errno << endl;
					#endif
					
					QSOCK_Print("}");
				}
			#endif
		#endif

		return SOCKET_ERROR;
	}

	// Reset various write/output variables:
	outbufferlen = 0;
	writeOffset = 0;

	// Clear the 'out-buffer'
	//clearOutBuffer(); // Commented out for various reasons.

	// Write to the console:
	#if defined(QSOCK_COUT_DETAILED) && QSOCK_COUT_DETAILED == 1
		QSOCK_Print("}");
	#endif

	// Return the default response.
	return 0;
}

bool QSocket::clearOutBuffer() { memset(outbuffer, 0, _bufferlen); outbufferlen = 0; writeOffset = 0; return true; }

// Buffer-writing related:

// The main write-command (Template-based).
bool QSocket::writeData(const void* input, uqint size)
{
	if ((outbufferlen+size) <= (QSOCK_UINT32)_bufferlen)
	{
		memcpy(outbuffer+outbufferlen, input, size);

		// Add to the buffer's length, and offset.
		writeOffset += size;
		outbufferlen += size;
	}
	else
	{
		// We were unable to write the data specified.
		return false;
	}

	// Return the default response.
	return true;
}

// Integer types:
bool QSocket::writeInt(QSOCK_INT32 data) { if (isServer()) data = ntohl(data); else data = htonl(data); return write<QSOCK_INT32>(&data); }

bool QSocket::writeByte(uqchar data) { return write<uqchar>(&data); }
bool QSocket::writeChar(qchar data) { return write<qchar>(&data); } // This is the same as 'writeByte'.
bool QSocket::writeShort(qshort data) { if (isServer()) data = ntohs(data); else data = htons(data); return write<qshort>(&data); }
bool QSocket::writeLong(QSOCK_INT64 data) { if (isServer()) data = ntohll(data); else data = htonll(data); return write<QSOCK_INT64>(&data); }

bool QSocket::writeBytes(const qchar* data, uqint dataSize) { return writeData(data, (dataSize == 0) ? (uqint)strlen((const QSOCK_CHAR*)data) : dataSize); }
bool QSocket::UwriteBytes(const uqchar* data, uqint dataSize) { return writeData(data, (dataSize == 0) ? (uqint)strlen((const QSOCK_CHAR*)data) : dataSize); }

// Floating point types:
bool QSocket::writeFloat(qfloat _data)
{ uqint data = htonf(_data); return write<uqint>(&data); }

bool QSocket::writeDouble(qdouble _data) { uqlong data = htond(_data); return write<uqlong>(&data); }

// Other types:

// Line related:
bool QSocket::writeLine(const QSOCK_CHAR* strIn, uqint length) { return writeLine((const unsigned QSOCK_CHAR*)strIn, length); }
bool QSocket::writeLine(const unsigned QSOCK_CHAR* strIn, uqint length) { return UwriteLine(strIn, length); }

bool QSocket::UwriteLine(const unsigned QSOCK_CHAR* strIn, uqint length)
{
	// Write all of the bytes in strIn to the 'outbuffer'.
	bool response = UwriteBytes((const uqchar*)strIn, length);
	if (!response) return response;

	// Setup the end of the line:
	response = writeChar('\r');
	if (!response) return response;
	
	response = writeChar('\n');

	// Return the response.
	return response;
}

// The rest of the commands:

// This is the same as closeSocket.
bool QSocket::close(QSOCK_INT32 nothing) { return closeSocket(); }

// Close the internal socket:
bool QSocket::closeSocket()
{
	// Namespace(s):
	using namespace std;

	// Write to the console:
	QSOCK_Print("");
	QSOCK_Print("QSocket -> closeSocket()" << endl << "{");

	// Check to see if the socket has been closed already.
	if (socketClosed)
	{
		QSOCK_Print("	QSocket -> closeSocket(): Failed - The socket has already been closed.");
		QSOCK_Print("}");

		return false;
	}

	// Delete any extra data, clean things up, etc:
	#if defined(QSOCK_IPVABSTRACT)
		freeaddrinfo((addrinfo*)result); //delete [] result;
	#else
		delete result;
	#endif
	
	result = NULL;
	boundAddress = NULL;

	ZeroVar(si_Destination);
	ZeroVar(so_Destination);

	// Free the internal socket:
	#if defined(_WIN32) || defined(_WIN64)
		if (_socket != 0)
		{
			if (closesocket(_socket) == SOCKET_ERROR)
			{
	#else
		if (_socket != 0)
		{
			if (close(_socket) == SOCKET_ERROR)
			{
	#endif
				#ifdef QSOCK_COUT
					#if QSOCK_COUT == 1
						cout << "	{?}";
					
						#if defined(_WIN32) || defined(_WIN64)
							cout << " {WINDOWS}";
						#endif

						cout << ": QSocket -> closeSocket(): ";
						#if defined(_WIN32) || defined(_WIN64)
							cout << "closesocket(): failed";
						#else
							cout << "close(): failed";
						#endif

						#if defined(_WIN32) || defined(_WIN64)
							cout << " - WSAGetLastError(): " << WSAGetLastError() << endl;
						#else
							cout << " - Error # " << errno << endl;
						#endif
					#endif
				#endif

				QSOCK_Print("}");

				return false;
			}
		}

	shutdownInternalSocket();

	// Set the socket as closed.
	socketClosed = true;

	// End the method for the output stream.
	QSOCK_Print("}");

	// Return the default response.
	return true;
}

// These commands are mainly here for Monkey, but you can use them in C++ as well:

#ifdef QSOCK_MONKEYMODE
	QSOCK_INT32 QSocket::StringToIntIP(nativeString IP)
#else
	QSOCK_UINT32_LONG QSocket::StringToIntIP(nativeString IP)
#endif
{
	// Definition(s):
	QSOCK_UINT32_LONG intIP = 0;
	
	// Calculate the integer for the IP address:
	#ifdef QSOCK_MONKEYMODE
		// Local variable(s):

		// Allocate a new character array.
		char* data = new char[IP.Length()+1];
		data[IP.Length()+1] = '\0';

		// Convert the monkey-based string into a c-string:
		
		// Convert the Monkey-native string to a character array:
		//wcstombs(data, IP.Data(), IP.Length());
		for (uqint index = 0; index < IP.Length(); index++)
			data[index] = IP[index];

		// Get the IPV4 address from 'data'.
		intIP = inet_addr(data);

		// Delete the temporary buffer.
		delete [] data;
	#else
		intIP = inet_addr(IP.c_str());
	#endif


	// Return the calculated IP address:
	return
	#ifdef QSOCK_MONKEYMODE
		(QSOCK_INT32)ntohl(intIP);
	#else
		ntohl(intIP);
	#endif
}

#ifdef QSOCK_MONKEYMODE
	nativeString QSocket::IntToStringIP(QSOCK_INT32 IP)
#else
	nativeString QSocket::IntToStringIP(QSOCK_UINT32_LONG IP)
#endif
{
	// Definition(s):
	struct in_addr address;

	// Add the IP to 'address':
	#if defined(_WIN32) || defined(_WIN64)
		address.S_un.S_addr =
	#else
		address.s_addr =
	#endif

	htonl(IP);

	// Return a nativeString of the IP address:
	#ifdef QSOCK_MONKEYMODE
		return nativeString(inet_ntoa(address));
	#else
		return inet_ntoa(address);
	#endif
}
