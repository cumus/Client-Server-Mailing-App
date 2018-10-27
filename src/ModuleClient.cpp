#include "ModuleClient.h"
#include "Log.h"
#include "imgui/imgui.h"
#include "serialization/PacketTypes.h"
#include "md5.h"

#define HEADER_SIZE sizeof(uint32_t)
#define RECV_CHUNK_SIZE 4096

bool ModuleClient::update()
{
	updateGUI();

	switch (state)
	{
	case ModuleClient::ClientState::Connecting:
		connectToServer();
		break;
	case ModuleClient::ClientState::Connected:
		handleIncomingData();
		updateMessenger();
		handleOutgoingData();
		break;
	case ModuleClient::ClientState::Disconnecting:
		disconnectFromServer();
		break;
	default:
		break;
	}

	return true;
}

bool ModuleClient::cleanUp()
{
	disconnectFromServer();
	return true;
}

void ModuleClient::updateMessenger()
{
	switch (messengerState)
	{
	case ModuleClient::MessengerState::SendingLogin:
		sendPacketLogin(senderBuf, passwordBuf);
		break;
	case ModuleClient::MessengerState::WaitingLoginResopnse:
		// Idle, do nothing
		break;
	case ModuleClient::MessengerState::RequestingClients:
		sendPacketQueryClients();
		break;
	case ModuleClient::MessengerState::ReceivingClients:
		// Idle, do nothing
		break;
	case ModuleClient::MessengerState::RequestingMessages:
		sendPacketQueryMessages();
		break;
	case ModuleClient::MessengerState::ReceivingMessages:
		// Idle, do nothing
		break;
	case ModuleClient::MessengerState::ShowingMessages:
		// Idle, do nothing
		break;
	case ModuleClient::MessengerState::ComposingMessage:
		// Idle, do nothing
		break;
	case ModuleClient::MessengerState::SendingMessage:
		sendPacketSendMessage(receiverBuf, subjectBuf, messageBuf);
		break;
	default:
		break;
	}
}

void ModuleClient::onPacketReceived(const InputMemoryStream & stream)
{
	PacketType packetType;
	stream.Read(packetType);

	LOG("onPacketReceived() - packetType: %s", packet_names[(int)packetType]);
	
	switch (packetType)
	{
	case PacketType::LoginResponse:
		onPacketReceivedLoginResponse(stream);
		break;
	case PacketType::QueryClientsResponse:
		onPacketReceivedQueryClientsResponse(stream);
		break;
	case PacketType::QueryAllMessagesResponse:
		onPacketReceivedQueryAllMessagesResponse(stream);
		break;
	default:
		LOG("Unknown packet type received");
		break;
	}
}

void ModuleClient::onPacketReceivedLoginResponse(const InputMemoryStream & stream)
{
	bool hasLoggedIn;
	stream.Read(hasLoggedIn);

	if (hasLoggedIn)
	{
		messengerState = MessengerState::RequestingClients;
	}
	else
	{
		hasLoginError = true;
		state = ClientState::Disconnecting;
	}
}

void ModuleClient::onPacketReceivedQueryClientsResponse(const InputMemoryStream & stream)
{
	registered_clients.clear();

	uint32_t clientCount;
	stream.Read(clientCount);

	for (uint32_t i = 0; i < clientCount; i++)
	{
		OtherClient client;
		stream.Read(client.name);
		stream.Read(client.state);
		registered_clients.push_back(client);
	}

	messengerState = MessengerState::RequestingMessages;
}

void ModuleClient::onPacketReceivedQueryAllMessagesResponse(const InputMemoryStream & stream)
{
	messages.clear();

	uint32_t messageCount;
	// TODO: Deserialize the number of messages
	stream.Read(messageCount);

	// TODO: Deserialize messages one by one and push_back them into the messages vector
	// NOTE: The messages vector is an attribute of this class
	for (uint32_t i = 0; i < messageCount; i++)
	{
		Message m;
		stream.Read(m.senderUsername);
		m.receiverUsername = senderBuf; // itself
		stream.Read(m.subject);
		stream.Read(m.body);
		messages.push_back(m);
	}

	messengerState = MessengerState::ShowingMessages;
}

void ModuleClient::sendPacketLogin(const char * username, const char * password)
{
	OutputMemoryStream stream;

	stream.Write(PacketType::LoginRequest);
	stream.Write(std::string(username));
	stream.Write(md5(password));

	sendPacket(stream);

	messengerState = MessengerState::WaitingLoginResopnse;
}

void ModuleClient::sendPacketQueryClients()
{
	OutputMemoryStream stream;
	stream.Write(PacketType::QueryClientsRequest);

	sendPacket(stream);

	messengerState = MessengerState::ReceivingClients;
}

void ModuleClient::sendPacketQueryMessages()
{
	OutputMemoryStream stream;
	stream.Write(PacketType::QueryAllMessagesRequest);

	sendPacket(stream);

	messengerState = MessengerState::ReceivingMessages;
}

void ModuleClient::sendPacketSendMessage(const char * receiver, const char * subject, const char *message)
{
	OutputMemoryStream stream;

	// TODO: Serialize message (packet type and all fields in the message)
	// NOTE: remember that senderBuf contains the current client (i.e. the sender of the message)
	stream.Write(PacketType::SendMessageRequest);
	stream.Write(std::string(senderBuf));
	stream.Write(std::string(receiver));
	stream.Write(std::string(subject));
	stream.Write(std::string(message));


	// TODO: Use sendPacket() to send the packet
	sendPacket(stream);

	messengerState = MessengerState::RequestingMessages;
}

// This function is done for you: Takes the stream and schedules its internal buffer to be sent
void ModuleClient::sendPacket(const OutputMemoryStream & stream)
{
	// Copy the packet into the send buffer
	size_t oldSize = sendBuffer.size();
	sendBuffer.resize(oldSize + HEADER_SIZE + stream.GetSize());
	uint32_t &packetSize = *(uint32_t*)&sendBuffer[oldSize];
	packetSize = HEADER_SIZE + stream.GetSize(); // header size + payload size
	//std::copy(stream.GetBufferPtr(), stream.GetBufferPtr() + stream.GetSize(), &sendBuffer[oldSize] + HEADER_SIZE);
	memcpy(&sendBuffer[oldSize] + HEADER_SIZE, stream.GetBufferPtr(), stream.GetSize());
}


// GUI: Modify this to add extra features...

void ModuleClient::updateGUI()
{
	if (chat_active && state == ClientState::Connected)
	{
		ImGui::Begin("Client Window");

		ImGui::TextWrapped("Username: %s", senderBuf);

		// Disconnect button
		if (ImGui::Button("Disconnect"))
			state = ClientState::Disconnecting;

		// Chat select
		std::vector<OtherClient>::iterator clients = registered_clients.begin();
		for (; clients != registered_clients.end(); clients++)
		{
			if (ImGui::Button(clients->name.c_str()))
			{
				// set new chat window reciever
				chat_reciever = clients->name;

				// add messages to chat
				chat_messages.clear();
				std::vector<Message>::iterator message = messages.begin();
				for (; message != messages.end(); message++)
				{
					if (message->senderUsername == chat_reciever)
						chat_messages.push_back({ true, message->body });
					else if (message->receiverUsername == chat_reciever)
						chat_messages.push_back({ false, message->body });
				}
			}
		}

		ImGui::End();

		// Online Chat Window
		if (chat_reciever.empty())
		{
			ImGui::Begin("Online Chat");
			ImGui::TextWrapped("Select a user to chat with");
		}
		else
		{
			ImGui::Begin(chat_reciever.c_str());

			ImGui::Columns(2, NULL, false);
			std::vector<std::pair<bool, std::string>>::iterator it;
			bool printing_on_left = true;
			for (it = chat_messages.begin(); it != chat_messages.end(); it++)
			{
				if (it->first != printing_on_left)
				{
					printing_on_left = it->first;
					ImGui::NextColumn();
				}

				ImGui::TextWrapped(it->second.c_str());
			}
			ImGui::Columns(1);
		}

		ImGui::End();
	}
	else
	{
		// Pos = 6, 7
		// Size = 234, 548
		ImGui::Begin("Client Window");

		if (state == ClientState::Disconnected)
		{
			if (ImGui::CollapsingHeader("Server data", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// IP address
				static char ipBuffer[64] = "127.0.0.1";
				ImGui::InputText("IP", ipBuffer, sizeof(ipBuffer));

				// Port
				static int port = 8000;
				ImGui::InputInt("Port", &port);

				// Credentials
				ImGui::InputText("Login name", senderBuf, sizeof(senderBuf));
				ImGui::InputText("Password", passwordBuf, sizeof(passwordBuf));

				// Connect button
				if (!std::string(passwordBuf).empty())
				{
					if (ImGui::Button("Connect"))
					{
						if (state == ClientState::Disconnected)
						{
							state = ClientState::Connecting;
						}
					}
				}
			}
		}
		else if (state == ClientState::Connected)
		{
			ImGui::TextWrapped("Username: %s", senderBuf);

			// Disconnect button
			if (ImGui::Button("Disconnect"))
				state = ClientState::Disconnecting;

			if (messengerState == MessengerState::ComposingMessage)
			{
				ImGui::InputText("Receiver", receiverBuf, sizeof(receiverBuf));
				ImGui::InputText("Subject", subjectBuf, sizeof(subjectBuf));
				ImGui::InputTextMultiline("Message", messageBuf, sizeof(messageBuf));
				if (ImGui::Button("Send"))
				{
					messengerState = MessengerState::SendingMessage;
				}
				if (ImGui::Button("Discard"))
				{
					messengerState = MessengerState::ShowingMessages;
				}
			}
			else if (messengerState == MessengerState::ShowingMessages)
			{
				if (ImGui::Button("Compose message"))
				{
					messengerState = MessengerState::ComposingMessage;
				}

				if (ImGui::Button("Refresh inbox"))
				{
					messengerState = MessengerState::RequestingMessages;
				}

				ImGui::Text("Inbox:");

				if (messages.empty()) {
					ImGui::Text(" - Your inbox is empty.");
				}

				int i = 0;
				for (auto &message : messages)
				{
					ImGui::PushID(i++);
					if (ImGui::TreeNode(&message, "%s - %s", message.senderUsername.c_str(), message.subject.c_str()))
					{
						ImGui::TextWrapped("%s", message.body.c_str());
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
		}

		ImGui::End();
	}
}


// Low-level networking stuff...

void ModuleClient::connectToServer()
{
	// Create socket
	connSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (connSocket == INVALID_SOCKET)
	{
		printWSErrorAndExit("socket()");
	}

	// Connect
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);
	int res = connect(connSocket, (const sockaddr*)&serverAddr, sizeof(serverAddr));
	if (res == SOCKET_ERROR)
	{
		printWSError("connect()");
		LOG("Could not connect to the server %s:%d", serverIP, serverPort);
		state = ClientState::Disconnecting;
	}
	else
	{
		state = ClientState::Connected;
		LOG("Server connected to %s:%d", serverIP, serverPort);

		messengerState = MessengerState::SendingLogin;
	}

	// Set non-blocking socket
	u_long nonBlocking = 1;
	res = ioctlsocket(connSocket, FIONBIO, &nonBlocking);
	if (res == SOCKET_ERROR) {
		printWSError("ioctlsocket() non-blocking");
		LOG("Could not set the socket in non-blocking mode.", serverIP, serverPort);
		state = ClientState::Disconnecting;
	}
}

void ModuleClient::disconnectFromServer()
{
	closesocket(connSocket);
	recvBuffer.clear();
	recvPacketHead = 0;
	recvByteHead = 0;
	sendBuffer.clear();
	sendHead = 0;
	state = ClientState::Disconnected;
}

void ModuleClient::handleIncomingData()
{
	if (recvBuffer.size() - recvByteHead < RECV_CHUNK_SIZE) {
		recvBuffer.resize(recvByteHead + RECV_CHUNK_SIZE);
	}

	int res = recv(connSocket, (char*)&recvBuffer[recvByteHead], RECV_CHUNK_SIZE, 0);
	if (res == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			// Do nothing
		}
		else
		{
			printWSError("recv() - socket disconnected forcily");
			state = ClientState::Disconnecting;
		}
	}
	else
	{
		if (res == 0)
		{
			state = ClientState::Disconnecting;
			LOG("Disconnection from server");
			return;
		}
		/*else if (res == 6)
		{
			state = ClientState::Disconnecting;
			LOG("Wrong Login Credentials");
			LOG("Disconnection from server");
			return;
		}*/

		recvByteHead += res;
		while (recvByteHead - recvPacketHead > HEADER_SIZE)
		{
			const size_t recvWindow = recvByteHead - recvPacketHead;
			const uint32_t packetSize = *(uint32_t*)&recvBuffer[recvPacketHead];
			if (recvWindow >= packetSize)
			{
				InputMemoryStream stream(packetSize - HEADER_SIZE);
				//std::copy(&recvBuffer[recvPacketHead + HEADER_SIZE], &recvBuffer[recvPacketHead + packetSize], (uint8_t*)stream.GetBufferPtr());
				memcpy(stream.GetBufferPtr(), &recvBuffer[recvPacketHead + HEADER_SIZE], packetSize - HEADER_SIZE);
				onPacketReceived(stream);
				recvPacketHead += packetSize;
			}
		}

		if (recvPacketHead >= recvByteHead)
		{
			recvPacketHead = 0;
			recvByteHead = 0;
		}
	}
}

void ModuleClient::handleOutgoingData()
{
	if (sendHead < sendBuffer.size())
	{
		int res = send(connSocket, (const char *)&sendBuffer[sendHead], (int)sendBuffer.size(), 0);
		if (res == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// Do nothing
			}
			else
			{
				printWSError("send()");
				state = ClientState::Disconnecting;
			}
		}
		else
		{
			sendHead += res;
		}

		if (sendHead >= sendBuffer.size())
		{
			sendHead = 0;
			sendBuffer.clear();
		}
	}
}