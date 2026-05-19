#include "ClientConnection.hpp"

ClientConnection::ClientConnection() : 
	fd(-1),
	state(READING_REQUEST),
	bytes_sent(0),
	request(NULL),
	response(NULL)
{
	std::cout << "ClientConnection created" << std::endl;
}

ClientConnection::~ClientConnection()
{
	std::cout << "ClientConnection destroyed, cleaning up..." << std::endl;
	std::cout << "___________________________________________"
		<< "___________________________________________" << std::endl;
	if (request)
	{
		delete request;
		request = NULL;
	}
	if (response)
	{
		delete response;
		response = NULL;
	}
}