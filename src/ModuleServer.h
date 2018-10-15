#pragma once

#include "Module.h"
#include "SocketUtils.h"
#include "serialization/MemoryStream.h"
#include <list>

class IDatabaseGateway;

class ModuleServer : public Module
{
public:

	// Constructor and destructor

	ModuleServer();

	~ModuleServer();


	// Virtual methods from parent class Module

	bool update() override;

	bool cleanUp() override;

private:

	// Methods involving serialization / deserialization (contain TODOs)

	void onPacketReceived(SOCKET socket, const InputMemoryStream& stream);

	void onPacketReceivedLogin(SOCKET socket, const InputMemoryStream& stream);

	void onPacketReceivedQueryAllMessages(SOCKET socket, const InputMemoryStream& stream);
	
	void onPacketReceivedSendMessage(SOCKET socket, const InputMemoryStream& stream);

	void sendPacketQueryAllMessagesResponse(SOCKET socket, const std::string &username);

	void sendPacket(SOCKET socket, OutputMemoryStream& stream);


	// GUI

	void updateGUI();


	// Low level networking stuff

	void startServer();

	void stopServer();

	void handleIncomingData();

	void handleOutgoingData();

	struct ClientStateInfo;

	void handleIncomingDataFromClient(ClientStateInfo &info);

	void handleOutgoingDataToClient(ClientStateInfo &info);

	std::vector<SOCKET> getAllSockets() const;


	// Client management

	void createClientStateInfoForSocket(SOCKET s);

	ClientStateInfo & getClientStateInfoForSocket(SOCKET s);

	bool existsClientStateInfoForSocket(SOCKET s);

	void deleteInvalidSockets();

	// Database

	IDatabaseGateway *database();


	// Data members

	enum class ServerState
	{
		Off,
		Starting,
		Running,
		Stopping
	};

	// Server state
	ServerState state = ServerState::Off;

	// Application port
	int port = 8000;

	// Special socket to accept incoming client connections.
	SOCKET listenSocket;

	// Client buffers
	struct ClientStateInfo
	{
		// Client socket
		SOCKET socket;

		// Recv buffer state
		size_t recvPacketHead = 0;
		size_t recvByteHead = 0;
		std::vector<uint8_t> recvBuffer;
	
		// Send buffer state
		size_t sendHead = 0;
		std::vector<uint8_t> sendBuffer;

		// Login
		std::string loginName;

		// bool should it be deleted?
		bool invalid = false;
	};

	// List with all connected clients
	std::list<ClientStateInfo> clients;

	// A gateway to database operations
	IDatabaseGateway *simulatedDatabaseGateway;
	IDatabaseGateway *mysqlDatabaseGateway;
};