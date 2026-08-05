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
	state(IDLE),
	bytesSent(0),
	request(NULL),
	_currentMethod(NULL),
	executor(NULL),
	responseBuilder(NULL),
	sessionManager(NULL),
	keep_alive(false),
	inactiveTime(std::time(NULL)),
	sendCookie(false),
	cgiStartTime(0),
	cgiPid(-1),
	cgiIn(-1),
	cgiOut(-1),
	cgiWrittenBytes(0),
	timeout(false)
{}

/**
 * @brief Deconstructs this object and the request object.
 */
ClientConnection::~ClientConnection()
{
	if (request) {
		delete request;
		request = NULL;
	}
	if (this->fd != -1) close(this->fd);
	if (this->cgiPid != -1) {
		kill(this->cgiPid, SIGKILL);
		waitpid(this->cgiPid, NULL, 0);
	}
	if (this->cgiIn != -1)
		close(this->cgiIn);
	if (this->cgiOut != -1)
		close(this->cgiOut);
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
	if (PRINT_PARSED_REQUEST && request->parsingComplete())
		request->printRequest();
	if (PRINT_LOCATION && request->getLocationObj())
		std::cout << BLUE << "location " << request->getLocationObj()->path << RESET << std::endl;
	if (PRINT_MODIFIED_URI)
		std::cout << BLUE << "mod_URI: " << request->getURI() << RESET << std::endl;
	if (request->validRequest()) {
		_currentMethod = executor->createMethod(request->getMethod(), request->getLocationObj());
		if (_currentMethod == NULL)
			response_buffer = responseBuilder->errorResponseViaCode(405);
		else if (request->getMethod() == "POST" && !request->hasBodyContentLength() && !request->getLocationObj()->cgi)
			response_buffer = responseBuilder->errorResponseViaCode(411);
		else {
			sessionHandling();
			executeRequest();
		}
	}
	else {
		response_buffer = responseBuilder->errorResponse(request, _listeningInterface);
	}
	if (state != CGI_PROCESSING)
		state = SENDING_RESPONSE;
	deleteMethod();
}

/**
 *
 */
void ClientConnection::cleanUpClient()
{
	state = IDLE;
	delete request;
	request = new HttpRequest();
	request->setServerConfigs(executor->getServerConfigs(), _listeningInterface);
	response_buffer = "";
	bytesSent = 0;
	sendCookie = false;
	cookieHeader = "";
	sessionCookie = "";
}

/**
 * @brief Request handling execution.
 */
void ClientConnection::executeRequest()
{
	t_executionResult result = executor->execute(_currentMethod, request);
	result.HttpVersion = request->getRequestLine().version;
	if (result.statusCode == "301" || result.statusCode == "302"){
		if (sendCookie)
			response_buffer = responseBuilder->redirectResponse(&result, _currentMethod->getRedirectURL(), cookieHeader);
		else
			response_buffer = responseBuilder->redirectResponse(&result, _currentMethod->getRedirectURL());
	}
	else {
		keep_alive = request->keepConnectionAlive();
		result.keep_alive = keep_alive;
		if (result.isCGI) {
			state = CGI_PROCESSING;
			cgi_path = result.cgiScriptPath;
		}
		else if (!result.success) {
			if (this->sendCookie)
				sessionManager->deleteSession(sessionCookie);
			response_buffer = responseBuilder->errorResponseViaResult(result, _listeningInterface);
		}
		else {
			if (this->sendCookie)
				response_buffer = responseBuilder->response(result, this->cookieHeader);
			else
				response_buffer = responseBuilder->response(result);
		}
	}
}

/**
 * @brief check for cookie-header and create new session if needed.
 */
void ClientConnection::sessionHandling()
{
	std::map<std::string, std::vector<std::string> > heads = request->getRequestHeaders();
	std::map<std::string, std::vector<std::string> >::iterator it = heads.find("cookie");
	std::string cookie;
	if (it != heads.end()) {
		cookie = sessionManager->extractCookie(it->second);
		if (!cookie.empty() && sessionManager->isValidCookie(cookie)) {
			sessionCookie = cookie;
		}
		else {
			sessionCookie = sessionManager->createNewSession();
			if (!sessionCookie.empty()) {
				this->sendCookie = true;
				this->cookieHeader = sessionManager->createSessionHeaderForResponse(sessionCookie);
			}
		}
	}
	else {
		sessionCookie = sessionManager->createNewSession();
		if (!sessionCookie.empty()) {
			this->sendCookie = true;
			this->cookieHeader = sessionManager->createSessionHeaderForResponse(sessionCookie);
		}
	}
}

void ClientConnection::deleteMethod()
{
	if (_currentMethod) {
		delete _currentMethod;
		_currentMethod = NULL;
	}
}

/**
 * @brief Extracts application-defined "X-Session-Set: key=value" headers from
 *  a raw CGI response, forwards each key/value pair into the generic
 *  session-data store, and strips the headers so they never reach the client.
 *  The server has no notion of what "key" means (login-state, cart, ...); it
 *  merely provides the storage mechanism for whatever the CGI script decides.
 */
void ClientConnection::applyCgiSessionHeaders()
{
	if (!sessionManager || sessionCookie.empty())
		return ;

	const std::string prefix = "X-Session-Set: ";
	size_t pos;
	std::cout << YELLOW << response_buffer << RESET << std::endl;
	while ((pos = response_buffer.find(prefix)) != std::string::npos)
	{
		size_t lineStart = pos + prefix.size();
		size_t lineEnd = response_buffer.find("\r\n", lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = response_buffer.size();

		std::string kv = response_buffer.substr(lineStart, lineEnd - lineStart);
		size_t eraseEnd = (lineEnd + 2 <= response_buffer.size()) ? lineEnd + 2 : lineEnd;
		response_buffer.erase(pos, eraseEnd - pos);

		size_t eq = kv.find('=');
		if (eq != std::string::npos) {
			std::string key = kv.substr(0, eq);
			std::string value = kv.substr(eq + 1);
			sessionManager->setData(sessionCookie, key, value);
		}
	}
}
