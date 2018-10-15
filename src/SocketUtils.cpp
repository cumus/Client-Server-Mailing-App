#include "SocketUtils.h"
#include <cstdio>
#include <cstdlib>

#include "Log.h"

// Link against this library
#pragma comment(lib, "ws2_32.lib")

void printWSError(const char *msg)
{
	wchar_t *s = NULL;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s,
		0,
		NULL);
	fprintf(stderr, "%s: %S\n", msg, s);
	LOG("%s: %S", msg, s);
	LocalFree(s);
}

void printWSErrorAndExit(const char *msg)
{
	printWSError(msg);
	system("pause");
	exit(-1);
}

void initializeSocketsLibrary()
{
	WSADATA wsaData;
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != NO_ERROR)
	{
		printWSErrorAndExit("WSAStartup");
	}
}

void cleanupSocketsLibrary()
{
	int res = WSACleanup();
	if (res != NO_ERROR)
	{
		// Let the app cleanup other stuff
		//printWSErrorAndExit("WSAStartup");
	}
}


std::vector<SOCKET> selectReadableSockets(const std::vector<SOCKET>& inputSockets)
{
	std::vector<SOCKET> selectedSockets;

	// Create a socket set wit all sockets
	fd_set set;
	FD_ZERO(&set);
	for (auto& socket : inputSockets) {
		FD_SET(socket, &set);
	}

	// Return immediately
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	// Select
	int numSocketsSelected = select(0, &set, nullptr, nullptr, &timeout);

	// Pick the selected sockets
	if (numSocketsSelected > 0)
	{
		for (auto& socket : inputSockets)
		{
			if (FD_ISSET(socket, &set))
			{
				selectedSockets.push_back(socket);
			}
		}
	}

	return selectedSockets;
}

std::vector<SOCKET> selectWritableSockets(const std::vector<SOCKET>& inputSockets)
{
	std::vector<SOCKET> selectedSockets;

	// Create a socket set wit all sockets
	fd_set set;
	FD_ZERO(&set);
	for (auto& socket : inputSockets) {
		FD_SET(socket, &set);
	}

	// Return immediately
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	// Select
	int numSocketsSelected = select(0, nullptr, &set, nullptr, &timeout);

	// Pick the selected sockets
	if (numSocketsSelected > 0)
	{
		for (auto& socket : inputSockets)
		{
			if (FD_ISSET(socket, &set))
			{
				selectedSockets.push_back(socket);
			}
		}
	}

	return selectedSockets;
}
