#include "ResponseBuilder.hpp"

// HELPERS
std::string getHttpDate();

ResponseBuilder::ResponseBuilder()
{}

ResponseBuilder::~ResponseBuilder()
{}

/**
	* @brief builds the response string, which will be send back to client.
	* @param result is the result of the executed Method.
	* @return full response-message. (status-line, headers, and message-body)
*/
std::string ResponseBuilder::formatResponse(t_executionResult result)
{
	std::string	resp;

	// build statusLine
	std::string statusLine = "HTTP/1.1 " + result.statusCode + " " + result.statusPhrase + "\r\n";
	// buildHeaders
	std::string messageHeaders;
	messageHeaders = setResponseHeaders(result);

	resp = statusLine + messageHeaders + result.body + "\r\n";
	return resp;
}

/**
	* @brief builds an error message-response based on the happend error.
*/
std::string	ResponseBuilder::buildErrorResponse(int errorCode)
{
	std::string response;
	std::string	statusCode;
	std::string	statusPhrase;
	std::string body;

	HttpStatus::setStatus(errorCode, statusCode, statusPhrase);
	response = "HTTP/1.1 " + statusCode + " " + statusPhrase + "\r\n";
	body = generateErrorPage(statusCode, statusPhrase);
	response += setErrorResponseHeaders(body.size());
	response += body;
	return response;
}

std::string	ResponseBuilder::setResponseHeaders(t_executionResult &result)
{
	std::string	messageHeaders;

	if (result.contentType.size() > 0)
	{
		std::stringstream ss;
		ss << result.body.size();

		messageHeaders = "Content-Type: " + result.contentType + "\r\n";
		messageHeaders += "Content-Length: " + ss.str() + "\r\n";
	}
	messageHeaders += "Date: " + getHttpDate() + "\r\n";
	messageHeaders += "Server: webserv/1.0\r\n";
	if (result.keep_alive)
		messageHeaders+= "Connection: keep-alive\r\n";
	else
		messageHeaders += "Connection: close\r\n";
	messageHeaders += "\r\n";
	return messageHeaders;
}

std::string ResponseBuilder::setErrorResponseHeaders(size_t contentLength)
{
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

std::string ResponseBuilder::generateErrorPage(const std::string &code, const std::string &phrase)
{
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
std::string getHttpDate()
{
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	char buf[100];

	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
	return std::string(buf);
}