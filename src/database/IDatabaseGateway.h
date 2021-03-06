#pragma once

#include "DatabaseTypes.h"
#include <vector>

class IDatabaseGateway
{
public:

	// Constructor and destructor

	IDatabaseGateway() { }
	
	virtual ~IDatabaseGateway() { }


	// Virtual methods

	virtual void insertMessage(const Message &message) = 0;

	virtual std::vector<Message> getAllMessagesFromUser(const std::string &username) = 0;

	virtual bool CheckPasswordForClient(const std::string &username, const std::string &password) = 0;

	virtual bool RegisterClient(const std::string &username, const std::string &password) = 0;

	virtual void updateGUI() { }
};

