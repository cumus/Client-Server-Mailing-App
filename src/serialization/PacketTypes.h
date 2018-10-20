#pragma once

#include <cstdint>

enum class PacketType : int8_t
{
	Empty,
	LoginRequest,
	LoginResponse,
	QueryAllMessagesRequest,
	QueryAllMessagesResponse,
	SendMessageRequest
};
