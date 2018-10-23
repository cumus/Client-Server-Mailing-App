#include "SimulatedDatabaseGateway.h"



SimulatedDatabaseGateway::SimulatedDatabaseGateway()
{
	// Load default users
	saved_clients["admin"] = "1234";
}


SimulatedDatabaseGateway::~SimulatedDatabaseGateway()
{
}

void SimulatedDatabaseGateway::insertMessage(const Message & message)
{
	allMessages.push_back(message);
}

std::vector<Message> SimulatedDatabaseGateway::getAllMessagesReceivedByUser(const std::string & username)
{
	std::vector<Message> messages;
	for (const auto & message : allMessages)
	{
		if (message.receiverUsername == username)
		{
			messages.push_back(message);
		}
	}
	return messages;
}

bool SimulatedDatabaseGateway::CheckPasswordForClient(const std::string & username, const std::string & password)
{
	bool ret = false;

	std::map<std::string, std::string>::iterator it = saved_clients.find(username);
	if (it != saved_clients.end()) // registered user
	{
		// check for correct password
		ret = (it->second == password);
	}
	else
	{
		// register new user
		saved_clients[username] = password;
		ret = true;
	}

	return ret;
}
