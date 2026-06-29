#include "HttpRequest.hpp"

size_t skipLWS(std::string val, size_t start, size_t end);
std::vector<std::string> splitHeaderValByComma(std::string val);

// =========================================================================
// Constructors & Destructor
// =========================================================================

/**
	* @brief Constructs the HttpRequest object.
	* @param _state Current state of parsing. (at request-line, headers, body)
*/
HttpRequest::HttpRequest(): 
	_state(PARSING_REQUEST_LINE), 
	_current_pos(0),
	_bodyParser(NULL),
	_errorCode(0)
{}

/**
	* @brief Deconstructs this object.
*/
HttpRequest::~HttpRequest()
{
	if (_bodyParser)
	{
		delete _bodyParser;
		_bodyParser = NULL;
	}
}

// =========================================================================
// Getters & Setters
// =========================================================================

s_RequestLine &HttpRequest::getRequestLine()
{
	return _requestLine;
}

std::map<
			std::string,
			std::vector<std::string> > &HttpRequest::getRequestHeaders()
{
	return _headers;
}

std::string	&HttpRequest::getRequestBody()
{
	return _fullMessageBody;
}

std::map<std::string, s_FormField> &HttpRequest::getParsedBody()
{
	return _parsedMessageBody;
}

t_ContentData	&HttpRequest::getContentData()
{
	return _contentData;
}

std::string &HttpRequest::getMethod()
{
	return _requestLine.method;
}

int HttpRequest::getErrorCode()
{
	std::cout << "HttpRequest::getErrorCode() => " << _errorCode << std::endl;
	return _errorCode;
}

std::string &HttpRequest::getURI()
{
	return _requestLine.requestURI;
}

std::string	HttpRequest::getRedirectLocation()
{
	std::cout << "HttpRequest::getRedirectLocatio()" << std::endl;
	std::string	location;
	std::map<std::string, std::vector<std::string> >::iterator it;

	std::cout << "\t_headers.size = " << _headers.size() << std::endl;
	it = _headers.find("Location");
	if (it != _headers.end())
		location = it->second[0];
	return location;
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
bool	HttpRequest::parseRequest(const std::string &partialMessage)
{
	std::cout << "\33[33m==========\n" 
		<< "HTTP_REQUEST received...\n" << std::endl;
	std::cout << "==========* PARSING REQUEST *==========" << std::endl;
	std::cout << "HttpRequest::parseRequest()" << std::endl;
	// std::cout << partialMessage << std::endl;
	_messageBuffer += partialMessage;
	// std::cout << _state << std::endl;
	while (_state != PARSING_COMPLETE && _state != PARSING_ERROR)
	{
		switch (_state)
		{
			case PARSING_REQUEST_LINE:
				if (!parseRequestLine())
					return true;
				_state = PARSING_HEADERS;
				break;
			case PARSING_HEADERS:
				if (!parseHeaderLine())
					return true;
				_state = PARSING_BODY;
				break;
			case PARSING_BODY:
				if (!extractBody())
					return true;
				if (_requestLine.method == "POST") // PARSING BODY
				{
					if (createBodyParser())
					{
						_bodyParser->setContentData(_contentData);
						_bodyParser->parse(_fullMessageBody);
						_parsedMessageBody = _bodyParser->getResult();
						if (_parsedMessageBody.size() > 0)
							std::cout << "_parsedBody returned something..." << std::endl;
					}
				}
				if (PRINT_REQUEST)
					printRequest();
				_state = PARSING_COMPLETE;
				break;
			case PARSING_ERROR:
				break;
			case PARSING_COMPLETE:
				break;
		}
	}
	return (_state != PARSING_ERROR);
}

bool HttpRequest::parsingComplete()
{
	if (_state == PARSING_COMPLETE || _state == PARSING_ERROR)
		return true;
	return false;
}

bool	HttpRequest::keepConnectionAlive()
{
	std::map<std::string, std::vector<std::string> >::iterator it;
	it = _headers.find("connection");
	if (it != _headers.end() && !it->second.empty())
	{
		if (it->second[0] == "keep-alive")
			return true;
	}
	return false;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

/**
	* @brief ignores empty lines before the request-line. Searches for the end 
	* of the request-line (CRLF -> "\r\n"). Extracts the 3 parts of a request-line.
	* (Method, requestURI and Http-Version).
	* @return false, if end of request-line not found.
	*         false & _state = PARSING_ERROR, if format is invalid.
	*         true, if request-line parsed successfully.
**/
bool	HttpRequest::parseRequestLine()
{
    while (_current_pos + 1 < _messageBuffer.size() &&
           _messageBuffer[_current_pos] == '\r' && 
           _messageBuffer[_current_pos + 1] == '\n')
    {
        _current_pos += 2;
	}
    size_t posCRLF = _messageBuffer.find("\r\n", _current_pos); // Find end of request line
    if (posCRLF == std::string::npos)
		return false;
	std::string reqLine = _messageBuffer.substr(_current_pos, posCRLF - _current_pos);
    if (reqLine.size() > MAX_REQUEST_LINE_LENGTH) // Check max length
    {
        _state = PARSING_ERROR;
		return setErrorCode(414);
    }
    size_t posSP1 = reqLine.find(' '); // Find first space (after METHOD)
	// std::cout << "SP1: " << posSP1 << std::endl;
    if (posSP1 == std::string::npos)
    {
        _state = PARSING_ERROR;
        return setErrorCode(400);
    }
    size_t posSP2 = reqLine.find(' ', posSP1 + 1); // Find second space (after URI)
    if (posSP2 == std::string::npos)
    {
        _state = PARSING_ERROR;
        return setErrorCode(400);
    }
	_requestLine.method = reqLine.substr(0, posSP1);
	_requestLine.requestURI = reqLine.substr(posSP1 + 1, posSP2 - posSP1 - 1);
	_requestLine.version = reqLine.substr(posSP2 + 1);

	_current_pos = posCRLF + 2;  // Skip \r\n
	// std::cout << "current_pos " << _current_pos << std::endl; 
	if (foundEndOfRequest())
	{
		std::cout << "request ends after request-line" << std::endl;
		_state = PARSING_COMPLETE;
		return false;
	}
	// size_t end = _messageBuffer.find("\r\n", posCRLF);

	return true;
}

/**
	* @brief Searches for the end of the header-section (\r\n\r\n).
	*  Extracts every header and parses into key:value (name:content).
	* @return false, if end of header-section not found.
	*         false & _state PARSING_ERROR, if error occured.
	*         true, if headers parsed successfully.
**/
bool HttpRequest::parseHeaderLine()
{
    size_t endOfHeaders = _messageBuffer.find("\r\n\r\n", _current_pos);
    if (endOfHeaders == std::string::npos)
        return false;
    while (_current_pos < endOfHeaders) // Parse each header line
    {
        size_t lineEnd = _messageBuffer.find("\r\n", _current_pos);
        if (lineEnd == std::string::npos || lineEnd > endOfHeaders)
            break;
        std::string line = _messageBuffer.substr(_current_pos, lineEnd - _current_pos);
        if (line.empty())
            break;
        if (_headers.size() >= MAX_HEADERS)
        {
            _state = PARSING_ERROR;
            return false;
        }
        if (line.size() > MAX_HEADER_LENGTH)
        {
            _state = PARSING_ERROR;
            return false;
        }
        size_t colonPos = line.find(':'); // find delimiter for key":"value
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
			toLowerCase(key); // header-field to lowercase
			size_t valueStart = colonPos + 1;// Skip ": " and any leading spaces
            while (valueStart < line.size() && line[valueStart] == ' ')
                valueStart++;
            std::string value = line.substr(valueStart);
			// (key = host || content-length) only one value allowed -> bad request(400).
			addHeader(key, value);
		}
		else
		{
			_state = PARSING_ERROR;
			setErrorCode(400);
		}
        _current_pos = lineEnd + 2;
    }
	_current_pos = endOfHeaders + 4;
	return true;
}

/**
	* @brief Extracts the message-body and parsses into std::string.
	* @return false, if not received full body.
	*         false & _state = PARSING_ERROR, if error occured.
	*         true, if message-body extracted successfully.
**/
bool	HttpRequest::extractBody()
{
    size_t contentLength = 0;
    std::map<std::string, std::vector<std::string> >::iterator it;

    it = _headers.find("content-length"); // Get Content-Length from headers
    if (it != _headers.end() && !it->second.empty())
    {
        contentLength = atoi(it->second[0].c_str());
        if (contentLength > MAX_BODY_SIZE)// Check body-size limit
        {
            _state = PARSING_ERROR;
            return setErrorCode(413);
        }
    }
    else
	{
        contentLength = 0;
	}
	if (_requestLine.method == "POST" && contentLength == 0)
	{
		_state = PARSING_ERROR;
		return setErrorCode(411);
	}
	size_t availableBytes = _messageBuffer.size() - _current_pos;
    if (availableBytes < contentLength)// full body received?
    {
        return false;
    }
    if (contentLength > 0)
    {
        _fullMessageBody = _messageBuffer.substr(_current_pos, contentLength);// Extract body
        _current_pos += contentLength;
    }
    else
        _fullMessageBody = "";
    return true;
}

/**
	* @brief Splits the header "ContentType" field into its type, subtype & boundary, to parse
	*  the message-Body later correctly.
	* @param value The header-content of [Content-Type].
	* @return type of Content (multipart, application, ...)
	* @example [ContentType] = multipart/... ; boundary=----geekboundary....
		type: "multipart"
		subtype: "..."
		boundary: "----geekboundary...."
*/
std::string HttpRequest::parseContentType(std::vector<std::string> value)
{
	std::cout << "parsing contentType..." << std::endl;
	std::string	temp;
	std::string	parameter;
	std::string type;
	size_t 		posSemiColon = 0;
	size_t 		posSlash = 0;

	temp = value.at(0);
	if ((posSlash = temp.find('/', 0)) < temp.size())
	{
		_contentData.type = temp.substr(0, posSlash);
		if ((posSemiColon = temp.find(';', posSlash)) < temp.size())
		{
			parameter = temp.substr(posSemiColon, temp.size() - posSemiColon);
			_contentData.subtype = temp.substr(posSlash + 1, posSemiColon - posSlash);
			size_t	posEqual = 0;
			if ((posEqual = parameter.find("=", 0)) < parameter.size())
			{
				_contentData.boundary = parameter.substr(posEqual + 1, parameter.size() - posEqual);
			}
		}
		else
		{
			_contentData.subtype = temp.substr(posSlash + 1, temp.size() - posSlash);
		}
		type = temp.substr(0, posSemiColon);
		parameter = temp.substr(posSemiColon + 1, temp.size());
	}
	return type;
}

/**
	* @brief Checks Content-Type header field and creates the appropiate
	*  body-parser. this->_bodyParser.
**/
bool HttpRequest::createBodyParser()
{
	std::cout << "HttpRequest::createBodyParser()" << std::endl;
	parseContentType(_headers["Content-Type"]);
	if (_contentData.type == "multipart")
	{
		std::cout << "Want to create MultipartParser" << std::endl;
		_bodyParser = createMultiParser();
	}
	else if (_contentData.type == "application")
	{
		std::cout << "Want to create FormParser" << std::endl;
		_bodyParser = createFormParser();
	}
	else if (_contentData.type.size() > 0)
		return setErrorCode(415);
	return true;
}

ABodyParser *HttpRequest::createMultiParser()
{
	return new MultipartParser();
}

ABodyParser *HttpRequest::createFormParser()
{
	return new FormParser();
}

bool HttpRequest::isImplementedMethod() // 501
{
	if (_requestLine.method == "GET"
		|| _requestLine.method == "POST"
		|| _requestLine.method == "DELETE")
		return true;
	return false;
}

bool HttpRequest::isHttpVersionSupported() // 505
{
	if (_requestLine.version == "HTTP/1.1")
		return true;
	return false;
}

bool HttpRequest::validRequest()
{
	std::cout << "HttpRequest::validRequest()" << std::endl;
	if (_errorCode != 0)
		return false;
	if (!isImplementedMethod())
		_errorCode = 501;
	if (!isHttpVersionSupported())
		_errorCode = 505;

    std::map<std::string, std::vector<std::string> >::iterator it;
    it = _headers.find("host");
	if (it == _headers.end() || it->second.size() == 0)
		_errorCode = 400;
	if (_requestLine.method == "POST")
	{
		it = _headers.find("content-length");
		if (it == _headers.end() || (it != _headers.end() && it->second[0] == "0"))
			_errorCode = 411;
	}
	if (_errorCode != 0)
	{
		std::cout << "\t==> FALSE" << std::endl;
		printRequest();
		return false;
	}
	std::cout << "\t==> TRUE" << std::endl;
	return true;
}

size_t skipLWS(std::string val, size_t start, size_t end)
{
	size_t cnt = 0;
	while (val[start] == ' ' && start < end)
	{
		cnt++;
		start++;
	}
	return (cnt);
}

std::vector<std::string> splitHeaderValByComma(std::string val)
{
	std::vector<std::string>	split;
	size_t	i = val.size();
	size_t	start = 0;
	size_t	end = 0;

	while(end < i)
	{
		end = val.find(',', start);
		size_t spaces = skipLWS(val, start, end);
		start+= spaces;
		if (end < i)
			split.push_back(val.substr(start, end - start));
		else if (end >= i)
		{
			split.push_back(val.substr(start, val.size()));
			break ;
		}
		start = end + 1;
	}
	return split;
}

void HttpRequest::printRequest(void)
{
	std::cout << "Request-Line:\n{" << std::endl;
	std::cout << "\tAMethod: [" << _requestLine.method << "]" << std::endl;
	std::cout << "\tPath: [" << _requestLine.requestURI << "]" << std::endl;
	std::cout << "\tVersion: [" << _requestLine.version << "]" << std::endl;
	std::cout << "}" << std::endl;

	std::map<std::string, std::vector<std::string> >::iterator it = _headers.begin();
	std::map<std::string, std::vector<std::string> >::iterator ite = _headers.end();
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
	std::cout << _fullMessageBody << "\n}" << std::endl;
}

/**
	* @brief Sets the _errorCode of this object to the arg code passed.
*/
bool HttpRequest::setErrorCode(int code)
{
	std::cout << "setErrorCode(" << code << ")\n";
	_errorCode = code;
	return false;
}

bool HttpRequest::foundEndOfRequest()
{
	size_t end = 0;

	end = _messageBuffer.find("\r\n\r\n", 0);
	if (end != std::string::npos)
	{
		if (end == _current_pos - 2)
		{
			std::cout << "HttpRequest::foundEndOfRequest() => TRUE" << std::endl;
			return true;
		}
	}
	// std::cout << "current_pos" << _current_pos << ", end of request" << end << std::endl;
	std::cout << "HttpRequest::foundEndOfRequest() => FALSE" << std::endl;
	return false;
}

std::string	HttpRequest::toLowerCase(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

void	HttpRequest::addHeader(const std::string &key, const std::string &value)
{
	std::map<std::string, std::vector<std::string> >::iterator it;
	it = _headers.find(key);
	if (it == _headers.end()) // new header-field
           _headers[key] = splitHeaderValByComma(value);
	else // header-field already exists
	{
		if (key == "host" || key == "content-length")
			setErrorCode(400);
		std::vector<std::string>	temp;
		temp = splitHeaderValByComma(value);
		for (size_t i = 0; i < temp.size(); i++)
			it->second.push_back(temp[i]);
	}
}