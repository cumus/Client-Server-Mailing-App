#pragma once

#include <cstdint>

enum class PacketType : int8_t
{
	Empty,
	LoginRequest,
	LoginResponse,
	QueryClientsRequest,
	QueryClientsResponse,
	QueryAllMessagesRequest,
	QueryAllMessagesResponse,
	SendMessageRequest
};
