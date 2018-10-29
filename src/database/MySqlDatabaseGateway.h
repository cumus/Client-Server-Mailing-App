#pragma once

#include "IDatabaseGateway.h"

class MySqlDatabaseGateway :
	public IDatabaseGateway
{
public:

	// Constructor and destructor

	MySqlDatabaseGateway();

	~MySqlDatabaseGateway();


	// Virtual methods from IDatabaseGateway

	void insertMessage(const Message &message) override;

	std::vector<Message> getAllMessagesFromUser(const std::string &username) override;

	bool CheckPasswordForClient(const std::string &username, const std::string &password) override;
	
	bool RegisterClient(const std::string &username, const std::string &password) override;

	void updateGUI() override;

private:

	// Text buffers for ImGUI
	char bufMySqlHost[64] = "citmalumnes.upc.es";
	char bufMySqlPort[64] = "3306";
	char bufMySqlDatabase[64] = "database";
	char bufMySqlTable[64] = "messages";
	char bufMySqlUsername[64] = "username";
	char bufMySqlPassword[64] = "password";
};

