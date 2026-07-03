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
	* @brief Stores the server-configurations
*/
bool ResponseBuilder::setConfig(t_Configs *serverConfig)
{
	_serverConfig = serverConfig;
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::setConfig() : server_name = " << _serverConfig->serverName << std::endl;
	return true;
}

/**
	* @brief builds the response string, which will be send back to client.
	* @param result is the result of the executed Method.
	* @return full response-message. (status-line, headers, and message-body)
*/
std::string ResponseBuilder::formatResponse(t_executionResult result)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::formatResponse()" << std::endl;
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
	std::string response;

	response = "HTTP/1.1 " + result->statusCode + " " + result->statusPhrase + "\r\n";
	response += "Location: " + redirectURL + "\r\n";
	response += "Content-Length: 0";
	response += "\r\n\r\n";
	return response;
}

/**
	* @brief Builds the Response after a cgi-execution.
*/
std::string ResponseBuilder::cgiFormation(const std::string &cgiBody)
{
	std::string resp;
	std::string	statusLine;
	std::string headers;
	statusLine = "HTTP/1.1 200 OK\r\n";
	std::string	messageHeaders;
	std::stringstream	ss;

	ss << cgiBody.size();
	messageHeaders += "Content-Length: " + ss.str() + "\r\n";
	messageHeaders += "Date: " + getHttpDate() + "\r\n";
	messageHeaders += "Server: webserv/1.0\r\n";
	messageHeaders+= "Connection: keep-alive\r\n\r\n";
	
	resp = buildFullResponse(statusLine, messageHeaders, cgiBody);
	return resp;
}

/**
	* @brief builds an error message-response based on the happend error.
*/
std::string	ResponseBuilder::buildErrorResponse(int errorCode)
{
	if (BUILDER_PRINT)
		std::cout << "ResponseBuilder::buildErrorResponse()" << std::endl;
	std::string	statusCode;
	std::string	statusPhrase;
	HttpStatus::setStatus(errorCode, statusCode, statusPhrase);

	std::string	body = generateErrorPage(statusCode, statusPhrase);
	std::string	response = "HTTP/1.1 " + statusCode + " " + statusPhrase + "\r\n";
	response += setErrorResponseHeaders(body.size());
	response += body;
	return response;
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

	statusLine = "HTTP/1.1 " + result->statusCode + " " + result->statusPhrase + "\r\n";
	return statusLine;
}

/**
	* @brief 
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
