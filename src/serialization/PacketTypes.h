#pragma once

#include <cstdint>

enum class PacketType : int8_t
{
	LoginRequest,
	QueryAllMessagesRequest,
	QueryAllMessagesResponse,
	SendMessageRequest
};
