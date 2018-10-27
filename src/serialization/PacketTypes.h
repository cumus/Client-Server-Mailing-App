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

static const char* packet_names[8] = {
	"Empty",
	"LoginRequest",
	"LoginResponse",
	"QueryClientsRequest",
	"QueryClientsResponse",
	"QueryAllMessagesRequest",
	"QueryAllMessagesResponse",
	"SendMessageRequest" };
