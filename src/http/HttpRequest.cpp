#include "HttpRequest.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

/**
	* @brief Constructs the HttpRequest object.
	* @param _state Current state of parsing. (at request-line, headers, body)
*/
HttpRequest::HttpRequest()
{
	data._state = PARSING_REQUEST_LINE;
	data._current_pos = 0,
	_bodyParser = NULL;
	data._errorCode = 0;
	data._locationObj = NULL;
	requestLineParser = new RequestLineParser(&data);
	requestHeadsParser = new RequestHeadsParser(&data);
	requestBodyParser = new RequestBodyParser(&data);
}

/**
	* @brief Deconstructs this object and its sub-objects.
*/
HttpRequest::~HttpRequest()
{
	std::cout << "HttpRequest deconstructed" << std::endl;
	if (_bodyParser) {
		delete _bodyParser;
		_bodyParser = NULL;
	}
	if (requestLineParser) {
		delete requestLineParser;
		requestLineParser = NULL;
	}
	if (requestHeadsParser) {
		delete requestHeadsParser;
		requestHeadsParser = NULL;
	}
	if (requestBodyParser) {
		delete requestBodyParser;
		requestBodyParser = NULL;
	}
}

// =========================================================================
// Public Methods
// =========================================================================

/**
	* @brief takes the message received by recv() and parses it into data
	*  structure. Changes the state after a part has been parsed.
	* @param partialMessage message received through recv() by server.
	* @return true if parsing is not complete,
	*  true and _state set to PARSING_ERROR if an error while parsing occured.
	* 	false when parsing is completed.
*/
bool	HttpRequest::parseRequest(const std::string &partialMessage, size_t bytesRecv)
{
	if (partialMessage.size() != bytesRecv) {
		data._state = PARSING_ERROR;
		return setErrorCode(400);
	}
	data._messageBuffer += partialMessage;
	std::cout << "Request: \n" << partialMessage << std::endl;
	while (data._state != PARSING_COMPLETE && data._state != PARSING_ERROR) {
		switch (data._state) {
			case PARSING_REQUEST_LINE:
				if (!requestLineParser->parseRequestLine())
					return true;
				data._state = PARSING_HEADERS;
				break;
			case PARSING_HEADERS:
				if (!requestHeadsParser->parseHeaderLine())
					return true;
				data._state = PARSING_BODY;
				break;
			case PARSING_BODY:
				if (!requestBodyParser->extractBody())
					return true;
				if (data._requestLine.method == "POST")
					requestBodyParser->parseBody();
				if (PRINT_PARSED_REQUEST)
					printRequest();
				data._state = PARSING_COMPLETE;
				break;
			case PARSING_ERROR:
				break;
			case PARSING_COMPLETE:
				break;
		}
	}
	return (data._state != PARSING_ERROR);
}

bool HttpRequest::parsingComplete()
{
	if (data._state == PARSING_COMPLETE || data._state == PARSING_ERROR)
		return true;
	return false;
}

bool	HttpRequest::keepConnectionAlive()
{
	std::map<std::string, std::vector<std::string> >::iterator it;
	it = data._headers.find("connection");
	if (it != data._headers.end() && !it->second.empty()) {
		if (it->second[0] == "keep-alive")
			return true;
	}
	if (data._requestLine.version == "HTTP/1.1")
		return (true);
	return false;
}

bool HttpRequest::validRequest()
{
	std::cout << "HttpRequest::validRequest()" << std::endl;
	if (data._errorCode != 0)
		return false;
	if (!isImplementedMethod())
		return setErrorCode(405);
	if (!isHttpVersionSupported())
		return setErrorCode(505);
	// if (!isValidURI(data._requestLine.requestURI))
	// 	return setErrorCode(403);
	if (!isValidHost())
		return false;
	if (!isValidPostRequest())
		return false;
	if (data._errorCode != 0) {
		std::cout << "\t==> FALSE" << std::endl;
		printRequest();
		return false;
	}
	std::cout << "\t==> TRUE" << std::endl;
	return true;
}

void	HttpRequest::setServerConfigs(std::vector<t_Configs> serverConfigs, const std::string &listeningInterface)
{
	data._serverConfigs = serverConfigs;
	data._listeningInterface = listeningInterface;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

void	HttpRequest::adjustCurrentPos(size_t pos)
{
	data._current_pos += pos;
}

void HttpRequest::setCurrentPos(size_t pos)
{
	data._current_pos = pos;
}

/**
	* @brief Sets the _errorCode of this object to the arg code passed.
*/
bool HttpRequest::setErrorCode(int code)
{
	std::cout << "setErrorCode(" << code << ")\n";
	data._errorCode = code;
	data._state = PARSING_ERROR;
	return false;
}

bool HttpRequest::foundEndOfRequest()
{
	size_t end = 0;

	end = data._messageBuffer.find("\r\n\r\n", 0);
	if (end != std::string::npos)
	{
		if (end == data._current_pos - 2)
		{
			std::cout << "HttpRequest::foundEndOfRequest() => TRUE" << std::endl;
			return true;
		}
	}
	// std::cout << "current_pos" << _current_pos << ", end of request" << end << std::endl;
	std::cout << "HttpRequest::foundEndOfRequest() => FALSE" << std::endl;
	return false;
}

bool	HttpRequest::isAllowedMethod()
{
	if (!data._locationObj)
		return (false);
	for (size_t i = 0; i < data._locationObj->allowedMethods.size(); i++) {
		if (data._requestLine.method == data._locationObj->allowedMethods[i])
			return true;
	}
	return false;
}

bool HttpRequest::isImplementedMethod() // 501
{
	if (data._requestLine.method == "GET"
		|| data._requestLine.method == "POST"
		|| data._requestLine.method == "DELETE"
		|| data._requestLine.method == "HEAD")
		return true;
	return false;
}

bool HttpRequest::isHttpVersionSupported() // 505
{
	if (data._requestLine.version == "HTTP/1.1" || data._requestLine.version == "HTTP/1.0")
		return true;
	return false;
}

bool	HttpRequest::isValidHost()
{
    std::map<std::string, std::vector<std::string> >::iterator it;
    it = data._headers.find("host");
	if (it == data._headers.end() || it->second.size() == 0) {
		data._errorCode = 400;
		return false;
	}
	else {
		data._host = it->second[0];
		for (size_t i = 0; i < data._host.size(); i++) {
			if (!isalnum(data._host[i]) && data._host[i] != '.' 
				&& data._host[i] != '/' && data._host[i] != ':') {
				data._errorCode = 400;
				return false;
			}
		}
		size_t posSemi;
		posSemi = data._host.find_last_of(':');
		if (posSemi != std::string::npos) {
			for (size_t i = posSemi + 1; i < data._host.size(); i++) {
				if (data._host[i] < '0' || data._host[i] > '9') {
					data._errorCode = 400;
					return false;
				}
			}
		}
	}
	return true;
}

bool	HttpRequest::isValidPostRequest()
{
    std::map<std::string, std::vector<std::string> >::iterator it;
//	REMOVE to avoid 411 if not allowed Method
	if (data._requestLine.method == "POST")
	{
		it = data._headers.find("content-length");
		if (it != data._headers.end() && it->second[0] == "0" && data._fullMessageBody.size() == 0) {
			data._errorCode = 200;
			return false;
		}
		it = data._headers.find("content-type");
		if (it != data._headers.end() && data._fullMessageBody.size() == 0) {
			data._errorCode = 400;
			return false;
		}
	}
	return true;
}

/**
	* @brief checks for path traversal (../../etc/passwd)
*/
// bool	HttpRequest::isValidURI(const std::string &uri)
// {
// 	if (uri.find("..") != std::string::npos
// 		|| uri.find("//") != std::string::npos)
// 		return false;
// 	return true;
// }

// =========================================================================
// Getters & Setters
// =========================================================================

s_RequestLine &HttpRequest::getRequestLine()
{
	return data._requestLine;
}

std::map<
			std::string,
			std::vector<std::string> > &HttpRequest::getRequestHeaders()
{
	return data._headers;
}

std::string	&HttpRequest::getRequestBody()
{
	return data._fullMessageBody;
}

std::map<std::string, s_FormField> &HttpRequest::getParsedBody()
{
	return data._parsedMessageBody;
}

t_ContentData	&HttpRequest::getContentData()
{
	return data._contentData;
}

std::string &HttpRequest::getMethod()
{
	return data._requestLine.method;
}

int HttpRequest::getErrorCode()
{
	return data._errorCode;
}

std::string &HttpRequest::getURI()
{
	return data._requestLine.requestURI;
}

std::string	HttpRequest::getRedirectLocation()
{
	std::string	location;
	std::map<std::string, std::vector<std::string> >::iterator it;

	it = data._headers.find("Location");
	if (it != data._headers.end())
		location = it->second[0];
	return location;
}

std::string &HttpRequest::getHost()
{
	return data._host;
}


std::string	HttpRequest::getFileExtension()
{
	return (data._fileExtension);
}

std::string	&HttpRequest::getPathInfo()
{
	return data._pathInfo;
}

std::string &HttpRequest::getScriptName()
{
	return data._scriptName;
}

std::string &HttpRequest::getPathTranslated()
{
	return data._pathTranslated;
}

t_Location	*HttpRequest::getLocationObj()
{
	return data._locationObj;
}

bool	HttpRequest::hasBodyContentLength()
{
	if (data._fullMessageBody.size() > 0)
		return true;
	return false;
}


// =========================================================================
// PRINT PARSED-REQUEST
// =========================================================================

void HttpRequest::printRequest(void)
{
	std::cout << "====================" << std::endl;
	std::cout << "Request-Line:\n{" << std::endl;
	std::cout << "\tAMethod: [" << data._requestLine.method << "]" << std::endl;
	std::cout << "\tPath: [" << data._requestLine.requestURI << "]" << std::endl;
	std::cout << "\tVersion: [" << data._requestLine.version << "]" << std::endl;
	std::cout << "}" << std::endl;

	std::map<std::string, std::vector<std::string> >::iterator it = data._headers.begin();
	std::map<std::string, std::vector<std::string> >::iterator ite = data._headers.end();
	std::cout << "Request-Headers:\n{" << std::endl;
	while (it != ite)
	{
		std::cout << "\t [" << it->first << "] = {";
		for (size_t i = 0; i < it->second.size(); i++)
		{
			if (i < it->second.size() - 1)
				std::cout << "\"" << it->second[i] << "\", ";
			else
				std::cout << "\"" << it->second[i] << "\"";
		}
		std::cout << "}" << std::endl;
		it++;
	}
	std::cout << "}" << std::endl;
	std::cout << "Request-Body:\n{" << std::endl;
	// std::cout.write(_fullMessageBody.c_str(), 100);
	if (data._fullMessageBody.size() < 100)
		std::cout << data._fullMessageBody;
	std::cout << "\n}" << std::endl;
	std::cout << "====================" << std::endl;
}