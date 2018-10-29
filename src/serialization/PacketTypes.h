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
	ReciecedNewMessage,
	UserConnected,
	SendMessageRequest
};

static const char* packet_names[10] = {
	"Empty",
	"LoginRequest",
	"LoginResponse",
	"QueryClientsRequest",
	"QueryClientsResponse",
	"QueryAllMessagesRequest",
	"QueryAllMessagesResponse",
	"ReciecedNewMessage",
	"UserConnected",
	"SendMessageRequest" };
