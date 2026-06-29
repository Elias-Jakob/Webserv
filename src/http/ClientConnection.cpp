#include "ClientConnection.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

/**
	* @brief Constructs the object, and sets needed data to default values.
	* @param fd Filedesktiptor of the Client-Connection.
	* @param state Representation of connection-state (read request, process or
	*  send response.
	* @param request Object where parsing of request-message happens.
*/
ClientConnection::ClientConnection() : 
	fd(-1),
	state(READING_REQUEST),
	bytesSent(0),
	request(NULL),
	_currentMethod(NULL),
	executor(NULL),
	responseBuilder(NULL),
	keep_alive(false),
	inactiveTime(0)
{
	std::cout << "ClientConnection created" << std::endl;
}

/**
	* @brief Deconstructs this object and the request object.
*/
ClientConnection::~ClientConnection()
{
	std::cout << "ClientConnection destroyed, cleaning up..." << std::endl;
	std::cout << "==========================================\n"
		<< "===========================================" << std::endl;
	if (request)
	{
		delete request;
		request = NULL;
	}
}

// =========================================================================
// Public Methods
// =========================================================================

/**
	* @brief Creates and Executes the requested Method. Then builds response
	*  and stores it inside the response_buffer.
	* state of this Object will be set to SENDING_RESPONSE.
*/
void ClientConnection::processRequest()
{
	std::cout << "ClientConnection::processRequest()" << std::endl;
	request->validRequest();
	if (request->getErrorCode() != 0)
	{
		std::cout << "ERROR occured: " << request->getErrorCode() << std::endl;
		response_buffer = responseBuilder->buildErrorResponse(request->getErrorCode());
	}
	else
	{
			std::cout << "==========* PARSED REQUEST *==========\n" << std::endl;
			std::cout << "==========* EXECUTING METHOD *==========" << std::endl;
		_currentMethod = executor->createMethod(request->getMethod(), request->getURI());
		if (_currentMethod != NULL)
		{
			t_executionResult result = executor->execute(_currentMethod, request);
			std::cout << "==========* EXECUTED METHOD *==========\n" << std::endl;
			std::cout << "==========* BUILDING RESPONSE * ==========" << std::endl;
			if (result.statusCode == "301") // REDIRECTION
			{
				response_buffer = responseBuilder->redirectResponse(&result, _currentMethod->getRedirectURL());
			}
			else if (result.statusCode == "601") {
				state = CGI_PROCESSING;
				cgi_path = result.statusPhrase;
			}
			else
			{
				response_buffer = responseBuilder->formatResponse(result);
				keep_alive = request->keepConnectionAlive();
				result.keep_alive = keep_alive;
			}
		}
		else
		{
			response_buffer = responseBuilder->buildErrorResponse(405);
		}
	}
	std::cout << "==========* BUILT RESPONSE *==========" << std::endl;
	state = SENDING_RESPONSE;
	
	if (_currentMethod)
	{
		delete _currentMethod;
		_currentMethod = NULL;
	}
}

/**
	*
*/
void ClientConnection::cleanUpClient()
{
	state = READING_REQUEST;
	delete request;
	request = new HttpRequest();
	response_buffer = "";
	bytesSent = 0;
}

// =========================================================================
// Getters & Setters
// =========================================================================

// =========================================================================
// Public Methods
// =========================================================================

// =========================================================================
// Private Helper Methods
// =========================================================================
