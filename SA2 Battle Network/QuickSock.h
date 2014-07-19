
/*
DISCLAIMER:

Copyright (c) 2014 - Anthony Diamond

Permission is hereby granted, free of charge, To any person obtaining a copy of this software And associated documentation files (the "Software"),
To deal in the Software without restriction, including without limitation the rights To use, copy, modify, merge, publish, distribute, sublicense,
And/Or sell copies of the Software, And To permit persons To whom the Software is furnished To do so, subject To the following conditions:

The above copyright notice And this permission notice shall be included in all copies Or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS Or IMPLIED, INCLUDING BUT Not LIMITED To THE WARRANTIES OF MERCHANTABILITY,
FITNESS For A PARTICULAR PURPOSE And NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS Or COPYRIGHT HOLDERS BE LIABLE For ANY CLAIM,
DAMAGES Or OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT Or OTHERWISE, ARISING FROM,
OUT OF Or IN CONNECTION WITH THE SOFTWARE Or THE USE Or OTHER DEALINGS IN THE SOFTWARE.
*/

// Data-type include(s):
#ifndef QSOCK_MONKEYMODE
	#include "QuickTypes.h"
#endif

// Only include once:
#if !defined(QSOCK_H)
	#define QSOCK_H

	/*
	This was used before, but now I'm just using
	'QSOCK_H' to be sure I don't have any issues:

	#ifndef QSOCK_MONKEYMODE
		#pragma once
	#endif
	*/

	// Preprocessor related:

	// QuickSock related:

	#if !defined(QSOCK_WINDOWS_LEGACY)
		#if (defined(_WIN32) || defined(_WIN64)) && defined(_MSC_VER) && (_MSC_VER <= 1600)
			#define QSOCK_WINDOWS_LEGACY 1
		#else
			#define QSOCK_WINDOWS_LEGACY 0
		#endif
	#endif

	#if !defined(QSOCK_DLL)
		#define QSOCK_DLL 0
	#endif

	#if defined(QSOCK_DLL)
		#if defined(QSOCK_MONKEYMODE) || !defined(_WIN32) && !defined(_WIN64)
			#undef QSOCK_DLL
			#define QSOCK_DLL 0
		#endif
	#endif

	#if !defined(QSOCK_COUT)
		#define QSOCK_COUT 0 // Set this to 1 if you need it (Either in here, or in your own file(s)).
	#endif

	#if defined(QSOCK_COUT) && QSOCK_COUT == 1
		#define QSOCK_Print(X) std::cout << X << std::endl
		#define QSOCK_CSTREAM(X) std::cout << X
	#elif !defined(QSOCK_COUT) || defined(QSOCK_COUT) && QSOCK_COUT == 0
		#define QSOCK_Print(X)
		#define QSOCK_CSTREAM(X)
	#endif

	#ifndef QSOCK_COUT_DETAILED
		#if defined(QSOCK_COUT) && QSOCK_COUT == 1
			#define QSOCK_COUT_DETAILED 1 // Set this to 1 if you need it (Either in here, or in your own file(s)).
		#else
			#define QSOCK_COUT_DETAILED 0
		#endif
	#endif

	#if !defined(QSOCK_IPVABSTRACT)
		//#define QSOCK_IPVABSTRACT // By defining this, 'QuickSock' will use a 'modern' abstract approach to IP-version handling (May cause problems).
		//#define QSOCK_IPV6 // IPV6 support is now handled with IPVABSTRACT.
	#endif

	#if defined(QSOCK_IPVABSTRACT)
		#if defined(QSOCK_IPV6)
			#undef QSOCK_IPV6
		#endif
	#endif

	#if !defined(QSOCK_AUTOINIT_SOCKETS)
		#if defined(_WIN32) || defined(_WIN64)
			#define QSOCK_AUTOINIT_SOCKETS 0 // If you enable this, be sure to call 'deinitSockets' when you're done.
		#else
			#define QSOCK_AUTOINIT_SOCKETS 1
		#endif
	#endif

	#if defined(QSOCK_DLL) && QSOCK_DLL == 1
		#define DLLImport   __declspec( dllimport )
		#define DLLExport   __declspec( dllexport )
	#else
		#define DLLExport
		#define DLLImport
	#endif

	#ifndef ZeroVar
		#define ZeroVar(X) memset(&X, 0, sizeof(X))
	#endif

	#ifndef QSOCK_ERROR
		// These can be found in the includes:
		#if defined(_WIN32) || defined(_WIN64)
			#define QSOCK_ERROR WSAGetLastError()
		#else
			#define QSOCK_ERROR errno
		#endif
	#endif

	// Includes:

	// Just a few standard libraries I need / may need in the future:
	/*
	#include <math.h> // Or "cmath"
	*/

	#include <string>
	#include <sstream>
	#include <stdlib.h> // Or "cstdlib"
	#include <stddef.h> // Or "cstddef"
	
	// If we're not in Monkey-mode, do the following:
	//#ifndef QSOCK_MONKEYMODE
	#if defined(QSOCK_COUT)
		// If we're using 'QSOCK_COUT', ensure that we include 'iostream':
		#if QSOCK_COUT == 1
			//#include <stdio.h>
			#include <iostream>
		#endif
	#endif

	// If we're using Windows, include 'WinSock', if not, include UNIX/BSD socket functionality:
	#if defined(_WIN32) || defined(_WIN64)
		// WinSock related:
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN
			#define WIN32_L_A_M_DEFINED
		#endif

		// Windows Includes (Much nicer than the Linux includes section):
		#include <windows.h>
		#include <winsock2.h>
		#include <ws2tcpip.h>
		#include <WSPiApi.h>
		#include <iphlpapi.h>

		#if defined(QSOCK_IPVABSTRACT)
			#include <mstcpip.h>
		#endif

		// Make sure to add the needed 'lib' file(s):
		#pragma comment (lib, "Ws2_32.lib")
		//#pragma comment (lib, "Mswsock.lib")
		//#pragma comment (lib, "AdvApi32.lib")

		#if defined(WIN32_L_A_M_DEFINED)
			#undef WIN32_L_A_M_DEFINED
			#undef WIN32_LEAN_AND_MEAN
		#endif

		// If for some odd reason we don't have this defined, make sure it is:
		/*
		#ifndef _WIN32_WINNT
			#define _WIN32_WINNT 0x501
		#endif
		*/
	#endif
	//#endif
	
	// Check if the (64-bit) byte-order commands we need are already supported, if not, define them:
	#ifndef WORDS_BIGENDIAN
		#if defined(__APPLE__) && defined(__MACH__) || defined(QSOCK_WINDOWS_LEGACY) && QSOCK_WINDOWS_LEGACY == 1 || defined(CFG_GLFW_USE_MINGW) && CFG_GLFW_USE_MINGW == 1 && defined(QSOCK_MONKEYMODE)
			// Host-To-Network 'long long' (64-bit integer).
			inline QSOCK_UINT64 htonll(QSOCK_UINT64 inInt)
			{
				// Local variable(s):

				// The 64-bit unsigned int used as a return-value.
				QSOCK_UINT64 retVal(0);

				// Pointers:
				uqchar* intToConvert = (uqchar*)&inInt;
				uqchar* returnInt = (uqchar*)&retVal;

				for (uqchar i = 0; i <= 7; i++)
					returnInt[i] = intToConvert[7-i];

				return retVal;
			}
	
			// Network-To-Host 'long long' (64-bit integer).
			inline QSOCK_UINT64 ntohll(QSOCK_UINT64 inInt)
			{
				// Local variable(s):

				// The 64-bit unsigned int used as a return-value.
				QSOCK_UINT64 retVal = 0;

				// Pointers:
				qchar* intToConvert = (qchar*)&inInt;
				qchar* returnInt = (qchar*)&retVal;
	
				for (uqchar i = 0; i <= 7; i++)
				{
					if (i != 7)
						returnInt[i] = intToConvert[7-i];
					else
						returnInt[i] = (intToConvert[7-i] >> 1);
				}
	
				return retVal;
			}
		#endif
	#endif

	// If we don't have prototypes for these byte-order related commands, declare them here:
	#ifndef WORDS_BIGENDIAN
		#if defined(QSOCK_WINDOWS_LEGACY) && QSOCK_WINDOWS_LEGACY == 1 || defined(CFG_GLFW_USE_MINGW) && CFG_GLFW_USE_MINGW == 1 && defined(QSOCK_MONKEYMODE)
			inline uqint htonf(const qfloat inFloat);
			inline qfloat ntohf(const QSOCK_UINT32_LONG inFloat);
			inline QSOCK_UINT64 htond(const QSOCK_FLOAT64 inFloat);
			inline qdouble ntohd(const QSOCK_UINT64 inFloat);
		#endif
	#endif

	#if !defined(_WIN32) && !defined(_WIN64)
		// Macros:
		#ifndef ZeroMemory
			#define ZeroMemory(X, Y) memset(X, 0, Y)
		#endif

		// Includes:

		// Oh god, what am I looking at:
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <sys/time.h>
		#include <netinet/in.h>
		#include <netdb.h>
		#include <unistd.h>
		#include <stdio.h>
		#include <arpa/inet.h>
		#include <stdint.h>
		#include <stdlib.h>
		#include <errno.h>

		// Don't mind me, just supporting things I don't need to:
		
		// Special thanks to Nanno Langstraat for this:
		#if defined(__linux__)
			// Includes:
			#include <endian.h>
		#elif defined(__FreeBSD__) || defined(__NetBSD__)
			// Includes:
			#include <sys/endian.h>
		#elif defined(__OpenBSD__)
			// Includes:
			//#include <sys/types.h>

			#define be16toh(x) betoh16(x)
			#define be32toh(x) betoh32(x)
			#define be64toh(x) betoh64(x)
		#endif

		// For the sake of laziness, I want to keep 'htonll' and 'ntohll':
		#if !defined(__APPLE__) && !defined(__MACH__)
			#define htonll(x) htobe64(x)
			#define ntohll(x) be64toh(x)
		#endif

		// Special thanks to Adam Banko for this part of the header.
		// In the end, I still needed to use this:
	
		//#ifndef QSOCK_MONKEYMODE
		// Based on code by Adam Banko:
		//#include "external/byteorder.h"

		// If we're compiling for a processor that's big-endian, disregard these commands:
		#ifdef WORDS_BIGENDIAN
			#define htons(x) (x)
			#define ntohs(x) (x)
			#define htonl(x) (x)
			#define ntohl(x) (x)
			#define htonf(x) (x)
			#define ntohf(x) (x)
			#define htond(x) (x)
			#define ntohd(x) (x)
		#else
			#ifdef PHP_WIN32
				#ifndef WINNT
					#define WINNT 1
				#endif
			#endif

			#ifndef htonf
				union float_long
				{
					qfloat f;
					qint l;
				};

				inline static qfloat htonf(qfloat x)
				{
					union float_long fl;

					fl.f = x;
					fl.l = htonl(fl.l);

					return fl.f;
				}

				inline static qfloat ntohf(qfloat x)
				{
					union float_long fl;

					fl.f = x;
					fl.l = htonl(fl.l);

					return fl.f;
				}
			#endif

			#ifndef htond

				#ifdef LINUX
					// Includes:
					#include <asm/byteorder.h>
					
					#define htond(x) __arch__swab64(x)
					#define ntohd(x) __arch__swab64(x)
				#else
					inline static QSOCK_FLOAT64 safe_swab64(QSOCK_FLOAT64 in)
					{
						QSOCK_FLOAT64 out;

						qchar* inP  = (qchar*)&in;
						qchar* outP = ((qchar*)&out) + sizeof(QSOCK_FLOAT64);

						for (uqint i=0; i< sizeof(QSOCK_FLOAT64); i++)
						{
							*(inP++) = *(--outP);
						}

						return out;
					}

					#define htond(x) safe_swab64(x)
					#define ntohd(x) safe_swab64(x)
				#endif
			#endif
		#endif
		//#endif
	#endif

	// Structures:
	/*
	struct timval
	{
		QSOCK_INT32_LONG tv_sec;
		QSOCK_INT32_LONG tv_usec;
	};
	*/

	// Typedefs/Aliases:
	typedef short nativePort;

	#if !defined(QSOCK_MONKEYMODE)
		typedef std::string nativeString;
	#else
		typedef String nativeString;
	#endif

	typedef wchar_t Char;

	#if !defined(QSOCK_IPVABSTRACT)
		typedef

		#ifdef QSOCK_MONKEYMODE
			QSOCK_UINT32_LONG
		#else
			QSOCK_INT32
		#endif

		nativeIP;

		typedef nativeString nonNativeIP;
	#else
		typedef nativeString nativeIP;
		typedef QSOCK_UINT32_LONG nonNativeIP;
	#endif

	// The address used internally by 'QuickSock':
	typedef

	#if !defined(QSOCK_IPVABSTRACT)
		sockaddr_in
	#else
		sockaddr
	#endif

	socketAddress;

	// Compatibility definitions:

	// If for some strange reason 'NULL' isn't defined, define it:
	#if !defined(NULL)
		// It should never come to this, but this is here just in case.
		#define NULL 0 // NEVER change this from zero.
	#endif

	// Just for the sake of laziness, we're keeping the WinSock naming-scheme for errors:
	#if !defined(WSAENETRESET)
		#define WSAENETRESET 10052
	#endif

	#if !defined(WSAECONNRESET)
		#define WSAECONNRESET 10054
	#endif

	#ifndef INET_ADDRSTRLEN
		#define INET_ADDRSTRLEN 22
	#endif

	#ifndef INET6_ADDRSTRLEN
		#define INET6_ADDRSTRLEN 65
	#endif

	#if !defined(_WIN32) && !defined(_WIN64)
		//#define closesocket close
	#endif

	#ifndef SOCKET_ERROR
		#define SOCKET_ERROR -1
	#endif

	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET -1
	#endif

	// Functions:

	// If we don't have 'max' for some reason, add it.
	#if !defined(max)
		#define max(a, b) (((a) > (b)) ? (a) : (b))
	#endif

	// Do the same for min.
	#if !defined(min)
		#define min(a, b) (((a) < (b)) ? (a) : (b))
	#endif

	// Classes:
	#ifndef QSOCK_MONKEYMODE
	class DLLExport QSocket
	{
	#else
	class QSocket : public Object
	{
	#endif
		public:
			// Global variables:

			// These variables dictate the amount of time spent detecting incoming packets:
			static const QSOCK_INT32_LONG TIMEOUT_USEC = 15;
			static const QSOCK_INT32_LONG TIMEOUT_SEC = 0;

			// The default max buffer-length for sockets.
			static const QSOCK_INT32 DEFAULT_BUFFERLEN = 1024;

			// The current default max buffer-length for sockets.
			static QSOCK_INT32 BUFFERLEN;

			// The length of the structure 'socketAddress' is set to.
			static QSOCK_INT32 socketAddress_Length;

			// A global boolean stating if sockets have been initialized or not.
			static bool socketsInitialized;

			// All meta-data specified by WinSock upon initialization (Windows only).
			#if defined(_WIN32) || defined(_WIN64)
				static WSADATA* WSA_Data;
			#endif

			// Constructors & Destructors:
			QSocket(QSOCK_INT32 bl=0);
			~QSocket();

			// Fields (Public):

			// The destinations used for sending and receiving packets.
			socketAddress si_Destination, so_Destination;

			// Functions (Public):
			static bool initSockets(QSOCK_INT32 bufferlen=DEFAULT_BUFFERLEN);
			static bool deinitSockets();

			// Methods (Public):

			// General methods:
			inline bool isServer() const { return _isServer; }
			inline bool isClosed() const { return socketClosed; }

			// Initialization related:
			bool connect(nativeString address, const nativePort ePort, nativePort iPort=(nativePort)0);

			// Return type/other ('connect').
			inline bool

			// Platform specific arguments:
			#if defined(QSOCK_MONKEYMODE)
			 connect(QSOCK_INT32 address, const QSOCK_INT32 ePort, const QSOCK_INT32 iPort
			#else
			 connect(QSOCK_INT32_LONG address, const nativePort ePort, const nativePort iPort
			#endif

			=(nativePort)0)
			{
				return connect(IntToStringIP(ntohl(htonl((QSOCK_UINT32_LONG)address))), (nativePort)ePort, (nativePort)iPort);
			}

			// Return type/other (directConnect).
			inline bool

			// Platform specific arguments:
			#if defined(QSOCK_MONKEYMODE)
			 directConnect(QSOCK_INT32 address, const QSOCK_INT32 ePort, const QSOCK_INT32 iPort
			#else
			 directConnect(QSOCK_INT32_LONG address, const nativePort ePort, const nativePort iPort
			#endif

			=(nativePort)0)
			{
				return connect(address, (nativePort)ePort, (nativePort)iPort);
			}

			bool host
			(
				#if defined(QSOCK_MONKEYMODE)
					const QSOCK_INT32 ePort
				#else
					const nativePort ePort
				#endif
			);

			// Update related:

			// These four do the same thing:
			inline QSOCK_INT32 updateSocket() { return listenSocket(); }
			inline QSOCK_INT32 update() { return listenSocket(); }
			inline QSOCK_INT32 listen() { return listenSocket(); }
			inline QSOCK_INT32 socketListen() { return listenSocket(); }

			QSOCK_INT32 listenSocket();

			inline QSOCK_INT32 hostUpdate();
			inline QSOCK_INT32 clientUpdate();

			// Internal (Don't use this unless you want platform dependant access):
			// This function is subject to change in the future.
			#if defined(_WIN32) || defined(_WIN64)
				inline SOCKET* getSocket() { return &_socket; }
			#else
				inline unsigned int getSocket() { return _socket; }
			#endif

			// Input related:
			QSOCK_INT32 readAvail();
			QSOCK_INT32 readMsg();

			inline uqchar* inMsgBuffer() const { return readMsgBuffer(); }
			inline uqchar* readMsgBuffer() const { return inbuffer; }

			bool clearInBuffer();

			inline QSOCK_INT32 msgSize() const { return inMsgLength(); }
			inline QSOCK_INT32 msgLength() const { return inMsgLength(); }
			inline QSOCK_INT32 msgOffset() const { return inMsgOffset(); }

			inline QSOCK_INT32 inMsgSize() const { return inMsgLength(); }
			QSOCK_INT32 inMsgLength() const { return max(inbufferlen, 0); }
			inline QSOCK_INT32 inMsgOffset() const { return readOffset; }

			inline bool msgAvail() const
			{
				return (msgSize() > 0);
			}

			inline nativeIP msgIP() const
			{
				return nativeMsgIP();
			}

			nativeString strMsgIP() const;
			QSOCK_UINT32_LONG intMsgIP() const;

			inline nativeIP nativeMsgIP() const
			{
				#if !defined(QSOCK_IPVABSTRACT)
					return intMsgIP();
				#else
					return strMsgIP();
				#endif
			}

			inline nonNativeIP nonNativeMsgIP()
			{
				#if !defined(QSOCK_IPVABSTRACT)
					return strMsgIP();
				#else
					return intMsgIP();
				#endif
			}

			nativePort msgPort() const;

			inline nativePort ntohs_msgPort() const
			{
				return msgPort();
			}

			inline nativePort htons_msgPort() const
			{
				return htons(msgPort());
			}

			// Return type/other ('msgAddr'):
			inline socketAddress msgAddr() const
			{
				return si_Destination;
			}
		
			// Buffer reading related:

			// Integer types:
			qshort readByte();
			qchar readChar();
			uqchar readOct();

			qshort readShort();
			QSOCK_INT32 readInt();
			QSOCK_INT64 readLong();

			// 'UreadBytes' produces a new 'uqchar' array, and should be managed/deleted by the caller.
			uqchar* UreadBytes(QSOCK_UINT32 count=0);

			// Like 'UreadBytes', 'readBytes' produces a new 'qchar' array, and should be managed/deleted by the caller.
			qchar* readBytes(QSOCK_UINT32 count=0);

			// General purpose:

			// 'resetRead' resets the read-offset to default/zero.
			inline void resetRead() { readOffset = 0; return; }

			// Floating point types:
			qfloat readFloat();
			qdouble readDouble();

			// Other types:
			inline nativeString readnativeString(QSOCK_UINT32 length=0)
			{
				// Local variable(s):
				QSOCK_CHAR* data = (QSOCK_CHAR*)UreadBytes(length);
				nativeString str = (nativeString)data;

				// Delete the temp-buffer.
				delete [] data;

				// Return the final 'nativeString'.
				return str;
			}
		
			// Line related:
			inline std::string readstdLine() { return readLine(); }
			nativeString readLine();

			// Output related:

			// This command only works on servers. Clients automatically call 'sendMsg':
			QSOCK_INT32 broadcastMsg(nativePort port=(nativePort)0);
			inline QSOCK_INT32 sendBroadcastMsg(nativePort port) { return broadcastMsg(port); }
		
			#if defined(QSOCK_MONKEYMODE)
				// Comments can be found below.
				QSOCK_INT32 sendMsg(QSOCK_INT32 IP, QSOCK_INT32 port=0);
				QSOCK_INT32 sendMsg(nativeString IP, QSOCK_INT32 port=0);
			#else
				// This overload is used for raw/native IPV4 addresses.
				QSOCK_INT32 sendMsg(QSOCK_UINT32_LONG IP, nativePort port=(nativePort)0);

				// This overload is used for string IP addresses. (IPV4, IPV6)
				QSOCK_INT32 sendMsg(nativeString strIP, nativePort port=(nativePort)0);
			#endif

			QSOCK_INT32 sendMsg(socketAddress* outboundAddress, uqint addressOffset=0)
			{
				if (!setupDestination(outboundAddress, addressOffset)) return SOCKET_ERROR;

				return outputMessage();
			}

			QSOCK_INT32 sendMsg();

			inline uqchar* writeMsgBuffer() const { return outMsgBuffer(); }
			inline uqchar* outMsgBuffer() const { return outbuffer; }

			bool clearOutBuffer();

			inline QSOCK_INT32 outMsgSize() { return outMsgLength(); }
			inline QSOCK_INT32 outMsgLength() { return max(outbufferlen, 0); }
			inline QSOCK_INT32 outMsgOffset() { return writeOffset; }

			// Buffer writing related:
			
			// The main write-command (Template-based).
			bool writeData(const void* input, uqint size=0);

			// These are just quick inline-wrappers for 'write':
			template<typename WType> inline bool write(WType* input, uqint size=0)
			{
				// If we don't have a specific size, use the size of 'WType'.
				if (size == 0)
					size = sizeof(WType);

				// Simply execute the main 'write' command.
				return writeData(input, size);
			}

			template<typename WType> inline bool write(WType input, uqint size=0)
			{
				return write<WType>(&input, size);
			}

			// Integer Types (Wrappers):
			bool writeInt(QSOCK_INT32 data);
			bool writeByte(uqchar data);
			bool writeChar(QSOCK_CHAR data);
			bool writeShort(qshort data);
			bool writeLong(QSOCK_INT64 data);
			
			bool UwriteBytes(const uqchar* data, uqint dataSize=0);
			bool writeBytes(const qchar* data, uqint dataSize=0);

			// General purpose:
			inline void resetWrite() { writeOffset = 0; return; }

			// Floating-point Types (Wrappers):
			bool writeFloat(qfloat data);
			bool writeDouble(qdouble data);

			// Other types:
			#if !defined(QSOCK_MONKEYMODE)
				inline bool writenativeString(nativeString str) { return writeBytes((const qchar*)str.c_str(), str.length()); }
			#endif

			inline bool writenativeString(const QSOCK_CHAR* str, uqint length=0) { return writeBytes((const qchar*)str, length); }
			inline bool writenativeString(const unsigned QSOCK_CHAR* str, uqint length=0) { return UwriteBytes((const uqchar*)str, length); }

			// Standard-line related:
			bool writeLine(const unsigned QSOCK_CHAR* strIn, uqint length=0);
			bool writeLine(const QSOCK_CHAR* strIn, uqint length=0);

			inline bool writeLine(std::string str) { return writeLine(str.c_str(), str.length()); }
			inline bool writestdLine(std::string str) { return writeLine(str); }

			// Here's the real 'UwriteLine' command.
			bool UwriteLine(const unsigned QSOCK_CHAR* strIn, uqint size=0);

			// The rest of the methods:
			bool closeSocket();
			bool close(QSOCK_INT32 nothing=0); // The 'nothing' argument was added due to problems with Unix sockets.
		
			// Monkey garbage collection and debugging related:
			#if defined(QSOCK_MONKEYMODE)
				QSocket* m_new()
				{
					#if defined(CFG_QSOCK_DEBUG_ENABLED) && CFG_QSOCK_DEBUG_ENABLED == 1
						DBG_ENTER("Native: QuickSock")
						//DBG_LOCAL((QSocket*)this, "Self")
						DBG_INFO("Unable to find location.");
					#endif
 
					return this;
				}

				void mark() { Object::mark(); return; }

				#if defined(CFG_QSOCK_DEBUG_ENABLED) && CFG_QSOCK_DEBUG_ENABLED == 1
					nativeString debug()
					{
						nativeString t = "(QSocket)\n";

						// Other:
						t += dbg_decl("Internal_Socket", &_socket);
						//t += dbg_decl("Port", &port);

						// Offsets:
						t += dbg_decl("ReadOffset", &readOffset);
						t += dbg_decl("WriteOffset", &writeOffset);

						// Flags/Booleans:
						t += dbg_decl("IsServer", &_isServer);
						t += dbg_decl("SocketClosed", &socketClosed);
						t += dbg_decl("Manually_Deleted", &manualDelete);
						t += dbg_decl("Broadcast_Supported", broadcastSupported)

						// Lengths:
						t += dbg_decl("In_Buffer_Length", &inbufferlen);
						t += dbg_decl("Out_Buffer_Length", &outbufferlen);
						t += dbg_decl("Maximum_Buffer_Length", &_bufferlen)

						return t;
					}

					nativeString dbg_type(QSocket**s) { return "QSocket"; }
				#endif
			#endif

			// Functions:

			// The real IP conversion commands:
			#ifdef QSOCK_MONKEYMODE
				static QSOCK_INT32 StringToIntIP(nativeString IP);
				static nativeString IntToStringIP(QSOCK_INT32 IP);
			#else
				static QSOCK_UINT32_LONG StringToIntIP(nativeString IP);
				static nativeString IntToStringIP(QSOCK_UINT32_LONG IP);
			#endif

			// These commands are here for the sake of compatibility:
			static inline QSOCK_UINT32_LONG StringToIntIP(QSOCK_UINT32_LONG IP)
			{
				// Return the input.
				return IP;
			}

			static inline nativeString IntToStringIP(nativeString IP)
			{
				// Return the input.
				return IP;
			}

			static inline nonNativeIP NativeToNonNativeIP(nativeIP input)
			{
				#if !defined(QSOCK_IPVABSTRACT)
					return IntToStringIP(input);
				#else
					return StringToIntIP(input);
				#endif
			}

			static inline nativeIP NonNativeToNativeIP(nonNativeIP input)
			{
				#if !defined(QSOCK_IPVABSTRACT)
					return StringToIntIP(input);
				#else
					return IntToStringIP(input);
				#endif
			}

			// This is just to keep with the C/C++ naming scheme:
			static inline QSOCK_UINT32_LONG stringToIntIP(nativeString IP)
			{
				// Return the main function.
				return StringToIntIP(IP);
			}

			static inline nativeString intToStringIP(QSOCK_UINT32_LONG IP)
			{
				return IntToStringIP(IP);
			}
		private:
			// Methods (Private):
			bool setupDestinationV4(QSOCK_UINT32_LONG address, nativePort externalPort);
			bool setupDestination(std::string address, nativePort externalPort);

			inline bool setupDestination(QSOCK_UINT32_LONG address, nativePort externalPort)
			{
				return setupDestinationV4(address, externalPort);
			}

			inline bool setupDestination(socketAddress* outboundAddress, uqint addressOffset=0)
			{
				memcpy(&so_Destination, outboundAddress+(addressOffset*socketAddress_Length), socketAddress_Length);

				// Return the default response.
				return true;
			}

			inline bool setupDestination()
			{
				return setupDestination(msgIP(), msgPort());
			}

			QSOCK_INT32 outputMessage();

			void setTimeValues(bool init=false);

			// Update methods:
			// Nothing so far.

			// Fields (Private):

			// Internal socket:
			#if defined(_WIN32) || defined(_WIN64)
				SOCKET _socket;
			#else
				unsigned int _socket;
			#endif

			// The input and output buffers:
			uqchar* inbuffer;
			uqchar* outbuffer;
		
			// Time-related field(s):
			timeval tv;

			// The connection port (Host: Local/Server port, Client: External / Remote server's port).
			nativePort port;

			// Flags/Booleans:
			// Nothing so far.

		protected:
			// Global variable(s):

			// The 'file-descriptor set' used for 'QSockets'.
			static fd_set fd;

			// Constructors & Destructors:
			bool setupObject(QSOCK_INT32 bl=0);
			bool freeObject();

			// Methods (Protected):

			// Initialization related:
			bool bindSocket(const nativePort port);
			qint bindInternalSocket(qint filter=-1);

			// Deinitialization related:
			inline qint shutdownInternalSocket()
			{
				qint response = 0;

				// Shutdown the internal socket.
				response = shutdown(_socket, SD_BOTH);

				if (response == 0)
				{
					// Set the internal socket to an "invalid socket".
					_socket = INVALID_SOCKET;
				}

				return response;
			}

			// Fields (Protected):

			// The final address-result(s):

			// 'boundAddress' points to the element of 'result' which is used to bind the internal socket.
			#if !defined(QSOCK_IPVABSTRACT)
				socketAddress * result, * boundAddress;
			#else
				addrinfo * result, * boundAddress;
			#endif

			// The hints used to evaluate addresses (Protocols, 'IP families', etc).
			struct addrinfo hints;

			// The length variables for each buffer:
			QSOCK_INT32 _bufferlen;
			QSOCK_INT32 inbufferlen;
			QSOCK_INT32 outbufferlen;

			// The read & write offsets:
			QSOCK_INT32 readOffset;
			QSOCK_INT32 writeOffset;

			// Flags/Booleans:

			// Currently doesn't do much, and it's inaccessible in most places.
			bool manualDelete;

			// A boolean describing the connection-mode; server or client.
			bool _isServer;

			// A simple boolean for the state of the socket.
			bool socketClosed;
			bool broadcastSupported;

			// Other:

			/*
			nativePort outputPort;
			nativeIP outputIP;
			*/
	};
#endif
