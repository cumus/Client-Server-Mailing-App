#include "ModuleServer.h"
#include "Log.h"
#include "imgui/imgui.h"
#include "serialization/PacketTypes.h"
#include "database/MySqlDatabaseGateway.h"
#include "database/SimulatedDatabaseGateway.h"


static bool g_SimulateDatabaseConnection = true;


#define HEADER_SIZE sizeof(uint32_t)
#define RECV_CHUNK_SIZE 4096


ModuleServer::ModuleServer()
{
	mysqlDatabaseGateway = new MySqlDatabaseGateway();
	simulatedDatabaseGateway = new SimulatedDatabaseGateway();

	// Load default users
	saved_clients["admin"] = "1234";
}

ModuleServer::~ModuleServer()
{
	delete mysqlDatabaseGateway;
	delete simulatedDatabaseGateway;
}

bool ModuleServer::update()
{
	updateGUI();

	switch (state)
	{
	case ServerState::Starting:
		startServer();
		break;
	case ServerState::Running:
		handleIncomingData();
		handleOutgoingData();
		deleteInvalidSockets();
		break;
	case ServerState::Stopping:
		stopServer();
		break;
	default:
		break;
	}

	return true;
}

bool ModuleServer::cleanUp()
{
	stopServer();

	return true;
}

void ModuleServer::onPacketReceived(SOCKET socket, const InputMemoryStream & stream)
{
	PacketType packetType = PacketType::Empty;

	// TODO: Deserialize the packet type
	stream.Read(packetType);

	LOG("onPacketReceived() - packetType: %d", (int)packetType);

	switch (packetType)
	{
	case PacketType::Empty:
		break;
	case PacketType::LoginRequest:
		onPacketReceivedLogin(socket, stream);
		break;
	case PacketType::QueryAllMessagesRequest:
		onPacketReceivedQueryAllMessages(socket, stream);
		break;
	case PacketType::SendMessageRequest:
		onPacketReceivedSendMessage(socket, stream);
		break;
	default:
		LOG("Unknown packet type received");
		break;
	}
}

void ModuleServer::onPacketReceivedLogin(SOCKET socket, const InputMemoryStream & stream)
{
	std::string loginName;
	// TODO: Deserialize the login username into loginName
	stream.Read(loginName);

	std::string password;
	stream.Read(password);

	bool client_logged = false;

	std::map<std::string, std::string>::iterator it = saved_clients.find(loginName);
	if (it != saved_clients.end()) // registered user
	{
		if (it->second == password) // correct password
			client_logged = true;
	}
	else
	{
		// register new user
		saved_clients[loginName] = password;
		client_logged = true;
	}

	if (client_logged)
	{
		// Register the client with this socket with the deserialized username
		ClientStateInfo & client = getClientStateInfoForSocket(socket);
		client.loginName = loginName;
	}

	sendPacketLoginResponse(socket, client_logged);
}

void ModuleServer::onPacketReceivedQueryAllMessages(SOCKET socket, const InputMemoryStream & stream)
{
	// Get the username of this socket and send the response to it
	ClientStateInfo & clientStateInfo = getClientStateInfoForSocket(socket);
	sendPacketQueryAllMessagesResponse(socket, clientStateInfo.loginName);
}

void ModuleServer::sendPacketQueryAllMessagesResponse(SOCKET socket, const std::string &username)
{
	// Obtain the list of messages from the DB
	std::vector<Message> messages = database()->getAllMessagesReceivedByUser(username);

	OutputMemoryStream outStream;
	// TODO: Create QueryAllMessagesResponse and serialize all the messages
	// -- serialize the packet type
	// -- serialize the array size
	// -- serialize the messages in the array
	outStream.Write(PacketType::QueryAllMessagesResponse);
	outStream.Write((int)messages.size());

	std::vector<Message>::iterator it = messages.begin();
	for (; it != messages.end(); it++)
	{
		outStream.Write(it->senderUsername);
		//outStream.Write(it->receiverUsername);
		outStream.Write(it->subject);
		outStream.Write(it->body);
	}

	// TODO: Send the packet (pass the outStream to the sendPacket function)
	sendPacket(socket, outStream);
}

void ModuleServer::sendPacketLoginResponse(SOCKET socket, const bool connected)
{
	OutputMemoryStream outStream;

	outStream.Write(PacketType::LoginResponse);
	outStream.Write(connected);

	sendPacket(socket, outStream);
}

void ModuleServer::onPacketReceivedSendMessage(SOCKET socket, const InputMemoryStream & stream)
{
	Message message;
	// TODO: Deserialize the packet (all fields in Message)
	stream.Read(message.senderUsername);
	stream.Read(message.receiverUsername);
	stream.Read(message.subject);
	stream.Read(message.body);

	// Insert the message in the database
	database()->insertMessage(message);
}

void ModuleServer::sendPacket(SOCKET socket, OutputMemoryStream & stream)
{
	ClientStateInfo & client = getClientStateInfoForSocket(socket);
	// Copy the packet into the send buffer
	size_t oldSize = client.sendBuffer.size();
	client.sendBuffer.resize(oldSize + HEADER_SIZE + stream.GetSize());
	uint32_t &packetSize = *(uint32_t*)&client.sendBuffer[oldSize];
	packetSize = HEADER_SIZE + stream.GetSize(); // header size + payload size
	//std::copy(stream.GetBufferPtr(), stream.GetBufferPtr() + stream.GetSize(), &client.sendBuffer[oldSize] + HEADER_SIZE);
	memcpy(&client.sendBuffer[oldSize] + HEADER_SIZE, stream.GetBufferPtr(), stream.GetSize());
}



// GUI: Modify this to add extra features...

void ModuleServer::updateGUI()
{
	ImGui::Begin("Server Window");

	if (state == ServerState::Off)
	{
		// Port
		ImGui::InputInt("Port", &port);

		// Simulate database
		ImGui::Checkbox("Simulate database", &g_SimulateDatabaseConnection);

		// To start the server
		if (ImGui::Button("Start server"))
		{
			if (state == ServerState::Off)
			{
				state = ServerState::Starting;
			}
		}
	}
	else if (state == ServerState::Running)
	{
		// To stop the server
		if (ImGui::Button("Stop server"))
		{
			if (state == ServerState::Running)
			{
				state = ServerState::Stopping;
			}
		}

		ImGui::Text("Connected clients:");
		for (auto & client : clients)
		{
			ImGui::Text(" - %s", client.loginName.c_str());
		}

		database()->updateGUI();
	}

	ImGui::End();
}


// Low-level networking stuff

void ModuleServer::startServer()
{
	// Create socket
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		printWSErrorAndExit("socket()");
	}

	// Force reuse address
	int enable = 1;
	int res = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&enable, sizeof(int));
	if (res == SOCKET_ERROR)
	{
		printWSErrorAndExit("setsockopt() SO_REUSEADDR");
	}

	// Bind
	sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	res = bind(listenSocket, (const sockaddr*)&bindAddr, sizeof(bindAddr));
	if (res == SOCKET_ERROR)
	{
		printWSErrorAndExit("bind()");
	}

	// Listen
	res = listen(listenSocket, 32);
	if (res == SOCKET_ERROR)
	{
		printWSErrorAndExit("listen()");
	}

	// Next state
	state = ServerState::Running;

	LOG("Sever listening port %d", port);
}

void ModuleServer::stopServer()
{
	for (auto & clientInfo : clients) {
		closesocket(clientInfo.socket);
	}

	clients.clear();

	closesocket(listenSocket);

	state = ServerState::Off;

	LOG("Server off");
}

void ModuleServer::handleIncomingData()
{
	std::vector<SOCKET> allSockets = getAllSockets();
	std::vector<SOCKET> readableSockets = selectReadableSockets(allSockets);

	for (auto& socket : readableSockets)
	{
		if (socket == listenSocket)
		{
			sockaddr_in clientAddr;
			int addrLen = sizeof(clientAddr);
			SOCKET connectedSocket = accept(listenSocket, (sockaddr*)&clientAddr, &addrLen);
			if (connectedSocket == INVALID_SOCKET)
			{
				printWSErrorAndExit("accept()");
			}
			createClientStateInfoForSocket(connectedSocket);
			LOG("New connection accepted");
		}
		else
		{
			ClientStateInfo & clientStateInfo = getClientStateInfoForSocket(socket);
			handleIncomingDataFromClient(clientStateInfo);
		}
	}
}

void ModuleServer::handleOutgoingData()
{
	std::vector<SOCKET> allSockets = getAllSockets();
	std::vector<SOCKET> writableSockets = selectWritableSockets(allSockets);

	for (auto& socket : writableSockets)
	{
		ClientStateInfo & clientStateInfo = getClientStateInfoForSocket(socket);
		handleOutgoingDataToClient(clientStateInfo);
	}
}

void ModuleServer::handleIncomingDataFromClient(ClientStateInfo & info)
{
	if (info.recvBuffer.size() - info.recvByteHead < RECV_CHUNK_SIZE) {
		info.recvBuffer.resize(info.recvByteHead + RECV_CHUNK_SIZE);
	}

	int res = recv(info.socket, (char*)&info.recvBuffer[info.recvByteHead], RECV_CHUNK_SIZE, 0);
	if (res == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			// Do nothing
		}
		else
		{
			printWSError("recv() - Error: Disconnecting client");
			LOG("recv() - Error: Disconnecting client: %s", info.loginName.c_str());
			info.invalid = true;
		}
	}
	else
	{
		if (res == 0)
		{
			LOG("Client disconnected flawlessly - client: %s", info.loginName.c_str());
			info.invalid = true;
			return;
		}

		info.recvByteHead += res;
		while (info.recvByteHead - info.recvPacketHead > HEADER_SIZE)
		{
			const size_t recvWindow = info.recvByteHead - info.recvPacketHead;
			const uint32_t packetSize = *(uint32_t*)&info.recvBuffer[info.recvPacketHead];
			if (recvWindow >= packetSize)
			{
				InputMemoryStream stream(packetSize - HEADER_SIZE);
				//std::copy(&info.recvBuffer[info.recvPacketHead + HEADER_SIZE], &info.recvBuffer[info.recvPacketHead + packetSize], (uint8_t*)stream.GetBufferPtr());
				memcpy(stream.GetBufferPtr(), &info.recvBuffer[info.recvPacketHead + HEADER_SIZE], packetSize - HEADER_SIZE);
				onPacketReceived(info.socket, stream);
				info.recvPacketHead += packetSize;
			}
		}
		if (info.recvPacketHead >= info.recvByteHead)
		{
			info.recvPacketHead = 0;
			info.recvByteHead = 0;
		}
	}
}

void ModuleServer::handleOutgoingDataToClient(ClientStateInfo & info)
{
	if (info.sendHead < info.sendBuffer.size())
	{
		int res = send(info.socket, (const char *)&info.sendBuffer[info.sendHead], (int)info.sendBuffer.size(), 0);
		if (res == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// Do nothing
			}
			else
			{
				printWSError("send() - Error: Disconnecting client");
				LOG("send() - Error: Disconnecting client: %s", info.loginName.c_str());
				info.invalid = true;
			}
		}
		else
		{
			info.sendHead += res;
		}

		if (info.sendHead >= info.sendBuffer.size())
		{
			info.sendHead = 0;
			info.sendBuffer.clear();
		}
	}
}

std::vector<SOCKET> ModuleServer::getAllSockets() const
{
	std::vector<SOCKET> allSockets;
	allSockets.push_back(listenSocket);
	for (auto& client : clients)
	{
		allSockets.push_back(client.socket);
	}
	return allSockets;
}

void ModuleServer::createClientStateInfoForSocket(SOCKET s)
{
	assert(!existsClientStateInfoForSocket(s) && "Cannot create more than one client per socket");
	ClientStateInfo clientStateInfo;
	clientStateInfo.socket = s;
	clientStateInfo.loginName = "<pending login>";
	clients.emplace_back(clientStateInfo);
}

ModuleServer::ClientStateInfo & ModuleServer::getClientStateInfoForSocket(SOCKET s)
{
	for (auto& clientStateInfo : clients)
	{
		if (s == clientStateInfo.socket)
		{
			return clientStateInfo;
		}
	}

	//assert(nullptr && "The client for this socket does not exist.");
}

bool ModuleServer::existsClientStateInfoForSocket(SOCKET s)
{
	for (auto& clientStateInfo : clients)
	{
		if (s == clientStateInfo.socket)
		{
			return true;
		}
	}
	return false;
}

void ModuleServer::deleteInvalidSockets()
{
	auto it = clients.begin();
	while (it != clients.end())
	{
		if (it->invalid)
		{
			closesocket(it->socket);
			it = clients.erase(it);
		}
		else
		{
			++it;
		}
	}
}

IDatabaseGateway * ModuleServer::database()
{
	if (g_SimulateDatabaseConnection) {
		return simulatedDatabaseGateway;
	} else {
		return mysqlDatabaseGateway;
	}
}
