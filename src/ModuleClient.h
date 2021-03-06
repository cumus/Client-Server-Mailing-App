#pragma once

#include "Module.h"
#include "SocketUtils.h"
#include "database/DatabaseTypes.h"
#include "serialization/MemoryStream.h"

class ModuleClient : public Module
{
public:
	// Virtual methods from parent class Module
	bool update() override;
	bool cleanUp() override;

private:

	// Methods involving serialization / deserialization (contain TODOs)
	void updateMessenger();

	void onPacketReceived(const InputMemoryStream &stream);
	void onPacketReceivedLoginResponse(const InputMemoryStream &stream);
	void onPacketReceivedQueryClientsResponse(const InputMemoryStream &stream);
	void onPacketReceivedQueryAllMessagesResponse(const InputMemoryStream &stream);
	void onPacketReceivedNewMessage(const InputMemoryStream &stream);
	void onPacketReceivedUserConnected(const InputMemoryStream &stream);

	void sendPacketLogin(const char *username, const char * password);
	void sendPacketQueryClients();
	void sendPacketQueryMessages();
	void sendPacketSendMessage(const char *receiver, const char *subject, const char *message);
	void sendPacket(const OutputMemoryStream &stream);
	
	// GUI
	void updateGUI();

	// Low-level networking stuff
	void connectToServer();
	void disconnectFromServer();

	void handleIncomingData();
	void handleOutgoingData();

	// Client connection state
	enum class ClientState
	{
		Disconnected,
		Connecting,
		Connected,
		Disconnecting
	};

	// State of the client
	ClientState state = ClientState::Disconnected;

	// IP address of the server
	char serverIP[32] = "127.0.0.1";

	// Port used by the server
	int serverPort = 8000;

	// Socket to connect to the server
	SOCKET connSocket;


	// Current screen of the messenger app
	enum class MessengerState
	{
		SendingLogin,
		WaitingLoginResopnse,
		RequestingClients,
		ReceivingClients,
		RequestingMessages,
		ReceivingMessages,
		ShowingMessages,
		ComposingMessage,
		SendingMessage
	};

	// Current screen of the messenger application
	MessengerState messengerState = MessengerState::SendingLogin;

	// All messages in the client inbox
	std::vector<Message> messages;

	// Composing Message buffers (for IMGUI)
	char senderBuf[64] = "loginName";   // Buffer for the sender
	char receiverBuf[64]; // Buffer for the receiver
	char subjectBuf[256]; // Buffer for the subject
	char messageBuf[4096];// Buffer for the message

	// Login Password
	char passwordBuf[64];   // Buffer for the password
	bool hasLoginError = false;

	// Send and receive buffers (low-level stuff)

	// Recv buffer state
	size_t recvPacketHead = 0;
	size_t recvByteHead = 0;
	std::vector<uint8_t> recvBuffer;

	// Send buffer state
	size_t sendHead = 0;
	std::vector<uint8_t> sendBuffer;

	// Online Chat
	std::vector<std::string> online_clients;

	bool chat_active = true;
	std::string chat_reciever;
	bool scroll_chat = false;
	std::vector<std::pair<bool, std::string>> chat_messages;
};
