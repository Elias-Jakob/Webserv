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
	inactiveTime(std::time(NULL)),
	cgiStartTime(0),
	cgiPid(-1),
	cgiIn(-1),
	cgiOut(-1),
	cgiWrittenBytes(0),
	timeout(false)
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
	if (this->cgiPid != -1)
		this->terminateCGIProcess();
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
	std::cout << "\t" << request->getErrorCode() << std::endl;
	if(request->validRequest())
	{
		std::cout << "==========* PARSED REQUEST *==========\n" << std::endl;
		_currentMethod = executor->createMethod(request->getMethod(), request->getURI(), _listeningInterface);
		if (_currentMethod == NULL)
			response_buffer = responseBuilder->errorResponseViaCode(405);
		else if (request->getMethod() == "POST" && !request->hasBodyContentLength())
			response_buffer = responseBuilder->errorResponseViaCode(411);
		else
			executeRequest();
		// if (_currentMethod != NULL)
			// executeRequest();
		// else
			// response_buffer = responseBuilder->errorResponseViaCode(405);
	}
	else
	{
		std::cout << "ERROR occured: " << request->getErrorCode() << std::endl;
		response_buffer = responseBuilder->errorResponse(request, _listeningInterface);
	}
	std::cout << "==========* BUILT RESPONSE *==========" << std::endl;
	if (state != CGI_PROCESSING)
		state = SENDING_RESPONSE;
	deleteMethod();
}

/**
	*
*/
void ClientConnection::cleanUpClient()
{
	state = READING_REQUEST;
	delete request;
	request = new HttpRequest();
	request->setServerConfigs(executor->getServerConfigs(), _listeningInterface);
	response_buffer = "";
	bytesSent = 0;
}

void	ClientConnection::terminateCGIProcess(std::map<int, ClientConnection&> *cgiPipes)
{
	int	status;
	
	kill(this->cgiPid, SIGKILL);
	waitpid(this->cgiPid, &status, WNOHANG);
	if (this->cgiIn != -1) close(this->cgiIn);
	if (this->cgiOut != -1) close(this->cgiOut);
	if (cgiPipes) {
		if (this->cgiIn != -1)
			cgiPipes->erase(this->cgiIn);
		if (this->cgiOut != -1)
			cgiPipes->erase(this->cgiOut);
	}
	this->cgiPid = this->cgiIn = this->cgiOut = -1;
	this->cgiWrittenBytes = 0;
}
/**
 * @brief Request handling execution.
 */
void	ClientConnection::executeRequest()
{
	t_executionResult result = executor->execute(_currentMethod, request, _listeningInterface);
	result.HttpVersion = request->getRequestLine().version;
	std::cout << "==========* EXECUTED METHOD *==========\n" << std::endl;
	if (result.statusCode == "301") // REDIRECTION
		response_buffer = responseBuilder->redirectResponse(&result, _currentMethod->getRedirectURL());
	else
	{
		std::cout << "IN ELSE" << std::endl;
		keep_alive = request->keepConnectionAlive();
		result.keep_alive = keep_alive;
		if (result.statusCode == "601")
		{
			result.statusCode = "200";
			state = CGI_PROCESSING;
			cgi_path = result.statusPhrase;
			result.statusPhrase = "OK";
		}
		else if (!result.success)
			response_buffer = responseBuilder->errorResponseViaResult(result, _listeningInterface);
		else
			response_buffer = responseBuilder->response(result);
	}
}

void ClientConnection::deleteMethod()
{	
	if (_currentMethod)
	{
		delete _currentMethod;
		_currentMethod = NULL;
	}
}
