#include "ResponseBuilder.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

ResponseBuilder::ResponseBuilder()
{}

ResponseBuilder::~ResponseBuilder()
{}

// =========================================================================
// Public Methods
// =========================================================================

/**
	* @brief builds the response string, which will be send back to client.
	* @param result is the result of the executed Method.
	* @return full response-message. (status-line, headers, and message-body)
*/
std::string ResponseBuilder::response(t_executionResult result)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::formatResponse()" << result.statusCode << std::endl;
	std::string statusLine = buildStatusLine(&result);
	std::string messageHeaders = buildResponseHeaders(result);
	std::string response = buildFullResponse(statusLine, messageHeaders, result.body);
	return response;
}

/**
	* @brief Builds the response if a Redirect is needed.
*/
std::string	ResponseBuilder::redirectResponse(
	t_executionResult *result, 
	const std::string &redirectURL)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::redirectResponse()" << std::endl;
	std::string	statusLine = buildStatusLine(result);

	std::string headers = "Location: " + redirectURL + "\r\n";
	headers += "Content-Length: 0\r\n\r\n";

	std::string response = buildFullResponse(statusLine, headers, "");
	return response;
}

/**
	* @brief Builds the Response after a cgi-execution.
*/
std::string ResponseBuilder::cgiResponse(const std::string &cgiBody)
{
	std::string resp;
	std::string	statusLine = "HTTP/1.1 200 OK\r\n";
	
	std::stringstream	ss;
	ss << cgiBody.size();

	statusLine = "HTTP/1.1 200 OK\r\n";

	std::string headers = "Content-Length: " + ss.str() + "\r\n";
	headers += "Date: " + getHttpDate() + "\r\n";
	headers += "Server: webserv/1.0\r\n";
	headers+= "Connection: keep-alive\r\n\r\n";
	
	resp = buildFullResponse(statusLine, headers, cgiBody);
	return resp;
}

/**
 * @brief Gets called if an error happend. 
 */
std::string	ResponseBuilder::errorResponse(HttpRequest *request, const std::string &listeningInterface)
{
	int	errorCode = request->getErrorCode();
	std::string	codeStr;
	std::string phrase;
	HttpStatus::setStatus(errorCode, codeStr, phrase);

	std::string status = statusLine(request->getRequestLine(), codeStr, phrase);

	std::string body = buildBody(errorCode, codeStr, phrase, listeningInterface);

	std::string headers = setErrorResponseHeaders(body.size());

	std::string response = buildFullResponse(status, headers, body);
	return response;
}

/**
	* @brief builds an error message-response based on the happend error.
*/
std::string	ResponseBuilder::errorResponseViaCode(int errorCode)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::buildErrorResponse()" << std::endl;

	std::string	statusCode;
	std::string	statusPhrase;
	HttpStatus::setStatus(errorCode, statusCode, statusPhrase);

	std::string	statusLine = "HTTP/1.1 " + statusCode + " " + statusPhrase + "\r\n";
	// std::string	statusLine = setErrorStatusLine(errorCode);
	std::string	body = generateErrorPage(statusCode, statusPhrase);

	std::string	headers = setErrorResponseHeaders(body.size());
	
	std::string	response = buildFullResponse(statusLine, headers, body);
	return response;
}

/**
 * @brief 
 */
std::string ResponseBuilder::errorResponseViaResult(t_executionResult result, const std::string &listeningInterface)
{
	int	errorCode = atoi(result.statusCode.c_str());
	std::string	codeStr;
	std::string phrase;
	HttpStatus::setStatus(errorCode, codeStr, phrase);

	// std::string status = statusLine(request->getRequestLine(), codeStr, phrase);
	std::string statusLine = buildStatusLine(&result);

	std::string body = buildBody(errorCode, codeStr, phrase, listeningInterface);

	std::string headers = setErrorResponseHeaders(body.size());

	std::string response = buildFullResponse(statusLine, headers, body);
	return response;
}

/**
	* @brief Stores the server-configurations
*/
bool ResponseBuilder::setConfig(std::vector<t_Configs> serverConfigs)
{
	_serverConfigs = serverConfigs;
	// if (BUILDER_PRINT)
	// 	std::cout << "ResponseBuilder::setConfig() : server_name = " 
	// 		<< _serverConfig->serverName << std::endl;
	return true;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

/**
	* @brief
*/
std::string ResponseBuilder::buildStatusLine(t_executionResult *result)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::buildStatusLine()" << std::endl;
	std::string statusLine;

	statusLine = result->HttpVersion + " " + result->statusCode + " " + result->statusPhrase + "\r\n";
	return statusLine;
}

/**
 * @brief 
*/
std::string	ResponseBuilder::setErrorStatusLine(int errorCode)
{
	std::string	code;
	std::string	phrase;
	HttpStatus::setStatus(errorCode, code, phrase);

	std::string	statusLine = "HTTP/1.1 " + code  + " " + phrase + "\r\n";
	return statusLine;
}

/**
	* @brief 
	*
*/
std::string	ResponseBuilder::buildResponseHeaders(t_executionResult &result)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::buildResponseHeaders()" << std::endl;
	std::string	messageHeaders;
	std::stringstream	ss;

	ss << result.body.size();
	if (result.contentType.size() > 0)
		messageHeaders = "Content-Type: " + result.contentType + "\r\n";
	messageHeaders += "Content-Length: " + ss.str() + "\r\n";
	messageHeaders += "Cache-Control: no-cache, no-store, must-revalidate\r\n";
	std::string	date = getHttpDate();
	if (!date.empty())
		messageHeaders += "Date: " + getHttpDate() + "\r\n";
	messageHeaders += "Server: webserv/1.0\r\n";
	if (result.keep_alive)
		messageHeaders+= "Connection: keep-alive\r\n";
	else
		messageHeaders += "Connection: close\r\n";
	// if (result.lastModified.size() > 0)
	// 	messageHeaders += "Last-Modified: " + result.lastModified + "\r\n";
	// if (result.etag.size() > 0)
	// 	messageHeaders += "ETag: " + result.etag + "\r\n";
	messageHeaders += "\r\n";
	return messageHeaders;
}

/**
	* @brief 
*/
std::string ResponseBuilder::buildFullResponse(
	const std::string &statusLine,
	const std::string &messageHeaders,
	const std::string &resultBody)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::buildFullResponse()" << std::endl;
	std::string	response;

	response = statusLine + messageHeaders + resultBody;
	return response;
}

/**
	* @brief 
*/
std::string ResponseBuilder::setErrorResponseHeaders(size_t contentLength)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::setErrorResponseHeaders()" << std::endl;
	std::string headers;
	std::stringstream ss;
	ss << contentLength;

	headers = "";
	headers += "Date: " + getHttpDate() + "\r\n";
	headers += "Server: webserv/1.0\r\n";
	headers += "Content-Length: " + ss.str() + "\r\n";
	// check if Connection should be closed.
	headers += "Connection: close\r\n";
	headers += "\r\n";
	return headers;
}

/**
	* @brief 
*/
std::string ResponseBuilder::generateErrorPage(const std::string &code, const std::string &phrase)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::generateErrorPage()" << std::endl;
	std::string body;

	body = "<!DOCTYPE html>\n";
    body += "<html>\n<head>\n";
    body += "<title>" + code + " " + phrase + "</title>\n";
    body += "<style>\n";
    body += "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }\n";
    body += "h1 { color: #d32f2f; }\n";
    body += "</style>\n</head>\n<body>\n";
    body += "<h1>" + code + " " + phrase + "</h1>\n";
    body += "<p>The requested resource could not be accessed.</p>\n";
    body += "<hr>\n<p><em>webserv/1.0</em></p>\n";
    body += "</body>\n</html>\n";
    return body;
}

/**
	* @brief extracts the current date-time in the format.
	* @return string representation of the current date and time.
*/
std::string ResponseBuilder::getHttpDate()
{
	time_t now = time(0);
	struct tm *tm; 
	tm = gmtime(&now);
	if (tm == NULL)
	{
		perror("gmtime:");
		return "";
	}
	char buf[100];

	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm);
	return std::string(buf);
}


/**
 * @brief checks the HTTP-Version
 */
bool	ResponseBuilder::isValidHttpVersion(const std::string &version)
{
	if (version.empty()
		|| (version != "HTTP/1.1" && version != "HTTP/1.0"))
		return false;
	return true;
}

/**
 * @brief builds the statusLine for the response
 */
std::string ResponseBuilder::statusLine(
	t_RequestLine reqLine, 
	const std::string &codeStr, 
	const std::string &phrase)
{
	std::string	version = reqLine.version;
	if (!isValidHttpVersion(version))
		version = "HTTP/1.1";

	std::string statusLine;
	statusLine = version + " " + codeStr + " " + phrase + "\r\n";
	return statusLine;
}

bool	ResponseBuilder::availableErrorPage(
	int errorCode, 
	std::map<int, 
	std::string>::iterator *itErrorPage,
	const std::string &listeningInterface)
{
	for (size_t i = 0; i < _serverConfigs.size(); i++)
	{
		for (size_t j = 0; j < _serverConfigs[i].listenInterfaces.size();j++)
		{
			if (_serverConfigs[i].listenInterfaces[j] == listeningInterface)
			{
				*itErrorPage = _serverConfigs[i].errorPages.find(errorCode);
				if (*itErrorPage != _serverConfigs[i].errorPages.end())
					return true;
			}
		}
	}
	return false;
}

/**
 * 
 */
std::string	ResponseBuilder::buildBody(int errorCode,
										const std::string &codeStr, 
										const std::string &phrase,
										const std::string &listeningInterface)
{
	std::cout << "ResponseBuilder::buildBody()" << std::endl;
	std::string	body;

	std::map<int, std::string>::iterator	itErrorPage;
	if (availableErrorPage(errorCode, &itErrorPage, listeningInterface))
	{
		std::cout << "\t SHOULD READ ERROR FILE" << std::endl;
		body = getErrorPage(itErrorPage);
		// body = "read error_page";
	}
	else
		body = generateErrorPage(codeStr, phrase);
	return body;
}

/**
 * 
 */
std::string	ResponseBuilder::getErrorPage(std::map<int, std::string>::iterator itErrorPage)
{
	std::cout << "ResponseBuilder::getErrorPage()" << std::endl;
	std::string pagePath = "./www" + itErrorPage->second;
	std::cout << "\tPath to error_page (" << pagePath << ")" << std::endl;
	struct stat	fileInfo;
	if (stat(pagePath.c_str(), &fileInfo) != 0)
	{
		std::cout << "stat failed" << std::endl;
	}
	std::ifstream	errorPageStream(pagePath.c_str(), std::ios::binary);
	if (!errorPageStream)
	{
		return "false";
	}
	std::string	buffer(fileInfo.st_size, '\0');
	errorPageStream.read(&buffer[0], fileInfo.st_size);
	std::string body = buffer;
	std::cout << "errorPage = " << body << std::endl;
	return buffer;
}