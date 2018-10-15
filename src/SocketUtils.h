#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>

void initializeSocketsLibrary();

void cleanupSocketsLibrary();

void printWSError(const char *msg);

void printWSErrorAndExit(const char *msg);

std::vector<SOCKET> selectReadableSockets(const std::vector<SOCKET> &inputSockets);

std::vector<SOCKET> selectWritableSockets(const std::vector<SOCKET> &inputSockets);
