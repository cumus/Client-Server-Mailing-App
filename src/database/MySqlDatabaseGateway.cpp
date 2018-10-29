#include "MySqlDatabaseGateway.h"
#include "DBConnection.h"
#include "../imgui/imgui.h"
#include <cstdarg>

// You can use this function to create the SQL statements easily, works like the printf function
std::string stringFormat(const char *fmt, ...)
{
	// Find out the final string length
	va_list ap;
	va_start(ap, fmt);
	int size = vsnprintf(nullptr, 0, fmt, ap);
	va_end(ap);

	// Format the actual resulting string
	std::string resultString;
	resultString.resize(size + 1, '\0');
	va_start(ap, fmt);
	vsnprintf(&resultString[0], resultString.size(), fmt, ap);
	va_end(ap);

	return resultString;
}


MySqlDatabaseGateway::MySqlDatabaseGateway()
{
}


MySqlDatabaseGateway::~MySqlDatabaseGateway()
{
}

void MySqlDatabaseGateway::insertMessage(const Message & message)
{
	DBConnection db(bufMySqlHost, bufMySqlPort, bufMySqlDatabase, bufMySqlUsername, bufMySqlPassword);

	if (db.isConnected())
	{
		DBResultSet res;

		std::string sqlStatement;
		// TODO: Create the SQL statement to insert the passed message into the DB (INSERT)
		sqlStatement = stringFormat("INSERT INTO message VALUES ('%s','%s','%s','%s')", message.senderUsername.c_str(), message.receiverUsername.c_str(), message.subject.c_str(), message.body.c_str());
		// insert some messages
		db.sql(sqlStatement.c_str());
	}
}

std::vector<Message> MySqlDatabaseGateway::getAllMessagesFromUser(const std::string & username)
{
	std::vector<Message> messages;

	DBConnection db(bufMySqlHost, bufMySqlPort, bufMySqlDatabase, bufMySqlUsername, bufMySqlPassword);

	if (db.isConnected())
	{
		std::string sqlStatement;
		// TODO: Create the SQL statement to query all messages from the given user (SELECT)
		sqlStatement = stringFormat("SELECT * FROM message WHERE receiver = '%s' OR sender = '%s'", username.c_str(), username.c_str());
		// consult all messages
		DBResultSet res = db.sql(sqlStatement.c_str());

		// fill the array of messages
		for (auto & messageRow : res.rows)
		{
			Message message;
			message.senderUsername = messageRow.columns[0];
			message.receiverUsername = messageRow.columns[1];
			message.subject = messageRow.columns[2];
			message.body = messageRow.columns[3];
			messages.push_back(message);
		}
	}


	return messages;
}

bool MySqlDatabaseGateway::CheckPasswordForClient(const std::string & username, const std::string & password)
{
	bool ret = false;

	DBConnection db(bufMySqlHost, bufMySqlPort, bufMySqlDatabase, bufMySqlUsername, bufMySqlPassword);

	if (db.isConnected())
	{
		std::string sqlStatement;
		// Create the SQL statement to query user
		sqlStatement = stringFormat("SELECT * FROM user WHERE name = '%s'", username.c_str());

		DBResultSet res = db.sql(sqlStatement.c_str());

		if (!res.rows.empty()) // registered user
		{
			// check for correct password
			if(res.rows[0].columns[1] == password) ret = true;
		}
		else
		{
			// register new user
			//Create the SQL statement to insert new client
			sqlStatement = stringFormat("INSERT INTO user VALUES ('%s','%s')", username.c_str(), password.c_str());

			db.sql(sqlStatement.c_str());
			ret = true;
		}

	}

	return ret;
}

bool MySqlDatabaseGateway::RegisterClient(const std::string & username, const std::string & password)
{
	return true;
}

void MySqlDatabaseGateway::updateGUI()
{
	ImGui::Separator();

	ImGui::Text("MySQL Server info");
	ImGui::InputText("Host", bufMySqlHost, sizeof(bufMySqlHost));
	ImGui::InputText("Port", bufMySqlPort, sizeof(bufMySqlPort));
	ImGui::InputText("Database", bufMySqlDatabase, sizeof(bufMySqlDatabase));
	ImGui::InputText("Username", bufMySqlUsername, sizeof(bufMySqlUsername));
	ImGui::InputText("Password", bufMySqlPassword, sizeof(bufMySqlUsername), ImGuiInputTextFlags_Password);
}
