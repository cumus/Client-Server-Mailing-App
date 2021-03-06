#pragma once

#include "IDatabaseGateway.h"
#include <vector>
#include <map>

class SimulatedDatabaseGateway :
	public IDatabaseGateway
{
public:

	// Constructor and destructor

	SimulatedDatabaseGateway();

	~SimulatedDatabaseGateway();


	// Virtual methods from IDatabaseGateway

	void insertMessage(const Message &message) override;

	std::vector<Message> getAllMessagesFromUser(const std::string &username) override;

	bool CheckPasswordForClient(const std::string &username, const std::string &password) override;

	bool RegisterClient(const std::string &username, const std::string &password) override;


private:

	std::vector<Message> allMessages;

	// Saved Clients
	std::map<std::string, std::string> saved_clients;
};
