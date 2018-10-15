#include "DBConnection.h"
#include "../Log.h"
#include <string>
#include <iostream>

#define SQL_RESULT_LEN 2048
#define SQL_RETURN_CODE_LEN 1024

DBConnection::DBConnection(
	const char *server,
	const char *port,
	const char *dataBase,
	const char *userName,
	const char *userPassword)
{
	connect(server, port, dataBase, userName, userPassword);
}

DBConnection::~DBConnection()
{
	disconnect();
}

bool DBConnection::isConnected() const
{
	return sqlConnHandle != NULL;
}

//static std::string quote(const std::string& quoteme)
//{
//	char *to = new char[quoteme.length() * 2 + 3];//3 = NULL + 2 quote chars
//	mysql_real_escape_string(&m_handle, to, quoteme.c_str(), quoteme.length());
//	std::string quotedvalue = std::string("'") + std::string(to) + std::string("'");
//	delete[] to;
//	return quotedvalue;
//}

static std::string buildQuery(const char *query, va_list &arguments)
{
	char *s;
	std::string str = std::string(query);
	for (size_t pos = 0; ; )
	{
		size_t spos = str.find("?S?", pos);
		size_t ipos = str.find("?#?", pos);
		pos = (spos < ipos && spos != std::string::npos) ? spos : ipos;
		if (pos == std::string::npos) break; // no more ?S?s in string
		s = va_arg(arguments, char *);
		std::string quoted_param = /*quote*/(std::string(s));
		str.replace(pos, 3, quoted_param);//3="?S?".length();
		pos += quoted_param.length();
	}
	return str;
}

DBResultSet DBConnection::sql(const char *query, ...)
{
	va_list arguments;
	va_start(arguments, query);
	std::string str = buildQuery(query, arguments);
	va_end(arguments);//if ?S? and params mismatch, it could cause a segfault

	DBResultSet resultSet;

	// Alloc statement handle
	SQLHSTMT sqlStatementHandle;
	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStatementHandle);

	// Process data  
	if (!(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)) {
		LOG("Could not allocate a statement handle.");
		return resultSet;
	}


	//output
	LOG("Executing SQL query: %s", query);

	//if there is a problem executing the query then exit application
	//else display query result
	//retcode = SQLExecDirect(sqlStatementHandle, (SQLCHAR*)"SELECT @@VERSION", SQL_NTS);
	retcode = SQLExecDirect(sqlStatementHandle, (SQLCHAR*)query, SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		SQLCHAR outSQLState[256];
		SQLINTEGER outNativeError;
		SQLCHAR outMessageText[256];
		SQLSMALLINT outMessageTextLen;
		SQLGetDiagRec(SQL_HANDLE_STMT, sqlStatementHandle, 1, outSQLState, &outNativeError, outMessageText, 256, &outMessageTextLen);
		LOG("Error querying SQL Server");
		LOG(" - SQL State: %s", outSQLState);
		LOG(" - Error Test: %s", outMessageText);
	}
	else
	{
		//declare output variable and pointer
		SQLCHAR columnValue[SQL_RESULT_LEN];
		SQLLEN ptrSqlVersion;
		while (SQLFetch(sqlStatementHandle) == SQL_SUCCESS)
		{
			DBResultRow resultRow;

			//display query result
			//std::cout << std::endl << "Query Result:" << std::endl;

			//traverse columns of the result
			SQLUSMALLINT column = 1;
			do
			{
				//get column result
				retcode = SQLGetData(sqlStatementHandle, column, SQL_CHAR, columnValue, SQL_RESULT_LEN, &ptrSqlVersion);
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					std::string resultColumn = (const char*)columnValue;
					resultRow.columns.push_back(resultColumn);
					//std::cout << std::endl << sqlVersion << std::endl;
				}

				column++;
			} while (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);


			resultSet.rows.push_back(resultRow);
		}
	}

	// Release the statement handle
	SQLFreeHandle(SQL_HANDLE_STMT, sqlStatementHandle);

	return resultSet;
}

bool DBConnection::connect(
	const char *server,
	const char *port,
	const char *dataBase,
	const char *userName,
	const char *userPassword)
{
	//initializations
	sqlEnvHandle = NULL;
	sqlConnHandle = NULL;

	// Allocate environment handle  
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle)) {
		disconnect();
		return false;
	}

	// Set the ODBC version environment attribute
	if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0)) {
		disconnect();
		return false;
	}

	// Allocate connection handle
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle)) {
		disconnect();
		return false;
	}

	// Set login timeout to 5 seconds 
	if (SQL_SUCCESS != SQLSetConnectAttr(sqlConnHandle, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0)) {
		disconnect();
		return false;
	}

	// desktop's window handle (to allow Window popups, could be NULL)
	HWND desktopHandle = GetDesktopWindow();

	// Fill the connection string
	std::string connectionString;
	connectionString += "DRIVER={MySQL ODBC 8.0 ANSI Driver};";
	connectionString += "SERVER=" + std::string(server) + ";";
	connectionString += "PORT=" + std::string(port) + ";";
	connectionString += "DATABASE=" + std::string(dataBase) + ";";
	connectionString += "USER=" + std::string(userName) + ";";
	connectionString += "PASSWORD=" + std::string(userPassword) + ";";
	connectionString += "OPTION=3";

	// Final connection string (more complete than CONN_STRING
	// when the user introduces extra info via popup window)
	SQLCHAR OutConnStr[1024];
	SQLSMALLINT OutConnStrLen;

	// Connect to the database
	SQLRETURN retcode = SQLDriverConnect( // SQL_NULL_HDBC  
		sqlConnHandle,
		desktopHandle,
		(SQLCHAR*)connectionString.c_str(),
		connectionString.size(),
		OutConnStr,
		1024,
		&OutConnStrLen,
		SQL_DRIVER_NOPROMPT); // With SQL_DRIVER_PROMPT, a window pops up!

	// Allocate statement handle  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		LOG("Connection succesful.");
		//std::cout << OutConnStr << endl; // Connection information, no need to print it!
		return true;
	}
	else // if (retcode == SQL_ERROR)
	{
		SQLCHAR outSQLState[256];
		SQLINTEGER outNativeError;
		SQLCHAR outMessageText[256];
		SQLSMALLINT outMessageTextLen;
		SQLGetDiagRec(SQL_HANDLE_DBC, sqlConnHandle, 1, outSQLState, &outNativeError, outMessageText, 256, &outMessageTextLen);
		LOG("Could not connect to the database.");
		LOG(" - SQL State: %s", outSQLState);
		LOG(" - Error Test: %s", outMessageText);
		disconnect();
		return false;
	}
}


void DBConnection::disconnect()
{
	SQLDisconnect(sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
}


void DBResultSet::print() const
{
	for (const auto& row : rows)
	{
		for (const auto& col : row.columns)
		{
			std::cout << "\"" << col << "\"\t\t\t";
		}

		std::cout << std::endl;
	}
}