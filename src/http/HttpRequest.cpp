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
	_errorCode(0),
	_locationObj(NULL)
{}

/**
	* @brief Deconstructs this object.
*/
HttpRequest::~HttpRequest()
{
	std::cout << "HttpRequest deconstructed" << std::endl;
	if (_bodyParser)
	{
		delete _bodyParser;
		_bodyParser = NULL;
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
	std::cout << "\33[33m==========\n" 
		<< "HTTP_REQUEST received...\n" << std::endl;
	std::cout << "==========* PARSING REQUEST *==========" << std::endl;
	std::cout << "HttpRequest::parseRequest()" << std::endl;

	if (partialMessage.size() != bytesRecv) {
		_state = PARSING_ERROR;
		return setErrorCode(400);
	}
	_messageBuffer += partialMessage;
	// std::cout << "[\n" << partialMessage << "\n]" << std::endl;
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
					if (createBodyParser()) {
						_bodyParser->setContentData(_contentData);
						std::cout << "body: " << _fullMessageBody << std::endl;
						_bodyParser->parse(_fullMessageBody);
						_parsedMessageBody = _bodyParser->getResult();
						if (_parsedMessageBody.size() > 0)
							std::cout << "_parsedBody returned something..." << std::endl;
					}
					else { // no content-type to create bodyParser but message-body present
						if (_fullMessageBody.size() >0) {
							std::map<std::string, s_FormField> res;
							s_FormField	body;
							body.value = _fullMessageBody;
							body.filename = "a";
							res["-"] = body;
							_parsedMessageBody = res;
							_state = PARSING_COMPLETE;
						}
						std::cout << "Error: BodyParsing" << std::endl;
						_state = PARSING_ERROR;
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
		if (it->second[0] == "keep-alive" ||
				(it->second[0] != "close" && _requestLine.version == "HTTP/1.1"))
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
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::parseRequestLine()" << std::endl;
    while (_current_pos + 1 < _messageBuffer.size() &&
           _messageBuffer[_current_pos] == '\r' && 
           _messageBuffer[_current_pos + 1] == '\n')
    {
        _current_pos += 2;
	}
    size_t posCRLF = _messageBuffer.find("\r\n", _current_pos); // Find end of request line
    if (posCRLF == std::string::npos){
		if ((_messageBuffer.find("\n", _current_pos)) != std::string::npos) {
			_state = PARSING_ERROR;
			return setErrorCode(400);
		}
		size_t posCR = _messageBuffer.find("\r", _current_pos);
		if (posCR != std::string::npos && posCR != _messageBuffer.size() -1) {
			_state = PARSING_ERROR;
			return setErrorCode(400);
		}
		return false;
	}
	std::string reqLine = _messageBuffer.substr(_current_pos, posCRLF - _current_pos);
    if (reqLine.size() > MAX_REQUEST_LINE_LENGTH) // Check max length
    {
        _state = PARSING_ERROR;
		return setErrorCode(414);
    }
    size_t posSP1 = reqLine.find(' '); // Find first space (after METHOD)
    if (posSP1 == std::string::npos)
    {
        _state = PARSING_ERROR;
        return setErrorCode(400);
    }
    size_t posSP2 = reqLine.find_last_of(' '); // Find second space (after URI)
    if (posSP2 == std::string::npos)
    {
        _state = PARSING_ERROR;
        return setErrorCode(400);
    }
	_requestLine.method = reqLine.substr(0, posSP1);
	_requestLine.requestURI = reqLine.substr(posSP1 + 1, posSP2 - posSP1 - 1);
	_requestLine.version = reqLine.substr(posSP2 + 1);
	if (_requestLine.method.size() < 1 || _requestLine.requestURI.size() < 1 || _requestLine.version.size() < 1)
	{
		_state = PARSING_ERROR;
		return setErrorCode(400);
	}
	size_t	posQuery = 0; // Query
	if (hasQuery(&posQuery))
		handleQuery(posQuery);
	// validate URI
	if (!validURI()) {
		_state = PARSING_ERROR;
		return false;
	}
	if (!decodeURI()) {
		_state = PARSING_ERROR;
		return setErrorCode(400);
	}
	// validate METHOD
	for (size_t i = 0; i < _requestLine.method.size(); i++) {
		if (_requestLine.method[i] < 'A' || _requestLine.method[i] > 'Z')
		{
			_state = PARSING_ERROR;
			return setErrorCode(400);
		}
	}
	// validate Http-version
	size_t pos_version;
	pos_version = _requestLine.version.find("HTTP/");
	if (pos_version == std::string::npos) {
		_state = PARSING_ERROR;
		return setErrorCode(400);
	}
	else if ((_requestLine.version[5] < '0' || _requestLine.version[5] > '9')
			|| (_requestLine.version[7] < '0' || _requestLine.version[7] > '9' )
			|| (_requestLine.version[6] != '.'))
	{
		_state = PARSING_ERROR;
		return setErrorCode(400);
	}

	extractFileExtension();
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

bool HttpRequest::decodeURI()
{
	std::cout << "HttpRequest::decodeURI()" << std::endl;
	size_t posPercent = _requestLine.requestURI.find('%');
	if (posPercent == std::string::npos)
		return true;
	else {
		std::cout << "\turi: " << _requestLine.requestURI << std::endl;
		size_t pos = 0;
		std::string newUri;
		while ((posPercent = _requestLine.requestURI.find('%', pos)) != std::string::npos) {
			newUri += _requestLine.requestURI.substr(pos, posPercent - pos);
			std::string hex = _requestLine.requestURI.substr(posPercent + 1, 2);
			char c = (char) std::strtol(hex.c_str(), NULL, 16);
			std::cout << "\tdecoded char: (" << c << ")" << std::endl;
			if (c < 32 || c > 127) // valid character in URI?
				return false;
			newUri += c;
			std::cout << "\tcur_URI: " << newUri << std::endl;
			pos = posPercent + 3;
		}
		newUri += _requestLine.requestURI.substr(pos);
		_requestLine.requestURI = newUri;
	}
	return true;
}

bool HttpRequest::validURIchar(char c)
{
	if ((c < 'A' || c > 'Z')
			&& (c < 'a' || c > 'z')
			&& (c < '0' || c > '9')
			&& (c != '/' && c != '.' && c != '_' && c != '%' && c != '-'))
		return false;
	return true;
}

bool HttpRequest::validURIstr(std::string &URI)
{
	for (size_t i = 0; i < URI.size(); i++) {
		if (!validURIchar(URI[i]))
			return false;
	}
	return true;
}

bool	HttpRequest::validURI()
{
	std::cout << "HttpRequest::validURI()" << std::endl;
	if (_requestLine.requestURI.size() > MAX_URI_LENGTH) {
		return setErrorCode(414);
	}
	if (!validURIstr(_requestLine.requestURI))
		return setErrorCode(400);
	size_t pos = 0;
	pos = _requestLine.requestURI.find("//");
	if (pos != std::string::npos)
		return setErrorCode(400);
	pos = _requestLine.requestURI.find("..");
	if (pos != std::string::npos)
		return setErrorCode(400);
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
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::parseHeaderLine()" << std::endl;
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
            return setErrorCode(431);
        }
        if (line.size() > MAX_HEADER_LENGTH)
        {
            _state = PARSING_ERROR;
            return setErrorCode(400);
        }
        size_t colonPos = line.find(':'); // find delimiter for key":"value
        if (colonPos != std::string::npos && colonPos != 0)
        {
            std::string key = line.substr(0, colonPos);
			toLowerCase(key); // header-field to lowercase
			size_t valueStart = colonPos + 1;// Skip ": " and any leading spaces
            while (valueStart < line.size() && line[valueStart] == ' ')
                valueStart++;
            std::string value = line.substr(valueStart);
			// check for unallowed chars in header-name and value
			for (size_t i = 0; i < key.size(); i++) {
				if (!isalpha(key[i]) && key[i] != '-') {
					_state = PARSING_ERROR;
					return setErrorCode(400);
				}
			}
			if (key.size() > MAX_HEADER_NAME_LENGTH || value.size() > MAX_HEADER_VALUE_LENGTH)
				return setErrorCode(400);
			// (key = host || content-length) only one value allowed -> bad request(400).
			addHeader(key, value);
			if (key == "host")
				_host = value;
		}
		else
		{
			_state = PARSING_ERROR;
			setErrorCode(400);
		}
        _current_pos = lineEnd + 2;
    }
	_current_pos = endOfHeaders + 4;

	std::vector<std::string> pathParts = splitPath(_requestLine.requestURI);
	findLocation(pathParts); // find corresponding t_Locatio
	
	std::cout << "LOCATION ==> " << _locationObj->path << std::endl;
	if (isAllowedMethod())
		std::cout << "\tMethod is allowed!" << std::endl;
	else
		std::cout << "\tMethod not allowed!" << std::endl;
	// modify requestURI, pathInfo, scriptName, pathTranslated.
	modifyURI(pathParts);
	return true;
}

// in MethodExecuter, we us _rootedLocs for this
void	HttpRequest::modifyURI(std::vector<std::string> &pathParts)
{
	std::cout << "MODIFY_URI()" << std::endl;
	if (!_locationObj->cgi)
	{
		size_t	idx_server;
		for (idx_server = 0; idx_server < _serverConfigs.size(); idx_server++) {
			if (isListeningTo(idx_server, _listeningInterface))
				break ;
		}
		std::string newURI;
		if (_serverConfigs[idx_server].root.size() > 0)
			newURI = _serverConfigs[idx_server].root;
		for (size_t iUri = 1; iUri < pathParts.size(); iUri++) {
			std::string part = pathParts[iUri];
			std::cout << "part: " << part << std::endl;
			for (size_t iLocs = 0; iLocs < _serverConfigs[idx_server].locations.size(); iLocs++) {
				if (pathParts[iUri] == _serverConfigs[idx_server].locations[iLocs].path) {
					if ( _serverConfigs[idx_server].locations[iLocs].upload)
						part = _serverConfigs[idx_server].locations[iLocs].uploadStore;
					else if (_serverConfigs[idx_server].locations[iLocs].alias.size() > 0)
						part = _serverConfigs[idx_server].locations[iLocs].alias;
					else if (_serverConfigs[idx_server].locations[iLocs].redirect)
						part = _serverConfigs[idx_server].locations[iLocs].redirectURL;
				}
			}
			newURI += part;
			std::cout << "newURI: " << newURI << std::endl;
		}
		if (_locationObj->formSubmit)
			newURI += "/" + _locationObj->formUploadFile;
		_requestLine.requestURI = newURI;
		std::cout << "\nmodified-URI: " << _requestLine.requestURI << "\n" << std::endl;
	}
	else if (_locationObj->cgi)
		modifyURIforCGI();
}

void HttpRequest::modifyURIforCGI()
{
	std::vector<std::string> pathParts = splitPath(_scriptName);
	size_t	idx_server;
	for (idx_server = 0; idx_server < _serverConfigs.size(); idx_server++) {
		if (isListeningTo(idx_server, _listeningInterface))
			break ;
	}
	std::cout << pathParts.size() << std::endl;
	std::string newURI;
	if (_serverConfigs[idx_server].root.size() > 0)
		newURI = _serverConfigs[idx_server].root;
	for (size_t iUri = 1; iUri < pathParts.size(); iUri++) {
		std::string part = pathParts[iUri];
		for (size_t iLocs = 0; iLocs < _serverConfigs[idx_server].locations.size(); iLocs++) {
			if (pathParts[iUri] == _serverConfigs[idx_server].locations[iLocs].path)
				part = _serverConfigs[idx_server].locations[iLocs].alias;
		}
		newURI += part;
	}
	_requestLine.requestURI = newURI;
	_scriptName = _requestLine.requestURI;
	// path_info
	pathParts = splitPath(_pathInfo);
	std::string	newPathInfo;
	for (size_t iUri = 1; iUri < pathParts.size(); iUri++) {
		std::string part = pathParts[iUri];
		for (size_t iLocs = 0; iLocs < _serverConfigs[idx_server].locations.size(); iLocs++) {
			if (pathParts[iUri] == _serverConfigs[idx_server].locations[iLocs].path)
				part = _serverConfigs[idx_server].locations[iLocs].alias;
		}
		newPathInfo += part;
	}
	_pathInfo = newPathInfo;
	_pathTranslated = _serverConfigs[idx_server].root + _pathInfo;
	std::cout << "\nSCRIPT_NAME: " << _scriptName 
		<< "\nPATH_INFO: " << _pathInfo 
		<< "\nPATH_TRANSLATED: " << _pathTranslated
		<< std::endl;
}

void	HttpRequest::findLocation(std::vector<std::string> pathParts)
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::findLocation()" << std::endl;

	t_Location	*loc = NULL;
	t_Location	*defLoc = NULL;
	bool		isCGI = false;

	for (size_t i = 0; i < _serverConfigs.size(); i++) {
		if (!isListeningTo(i, _listeningInterface))
			continue ;
		for (size_t k = 0; k < pathParts.size(); k++) {
			for (size_t j = 0; j < _serverConfigs[i].locations.size(); j++) {
				if (!defLoc && _serverConfigs[i].locations[j].path == "/")
					defLoc = &_serverConfigs[i].locations[j];
				if (_serverConfigs[i].locations[j].path == pathParts[k])
				{
					loc = &_serverConfigs[i].locations[j];
					if (loc && loc->cgi && isCGI == false) {
						isCGI = true;
						size_t posScript = posOfScriptName(pathParts, loc->cgiExtensions, k + 1);
						setScriptName(pathParts, posScript);
						setPathInfo(pathParts, posScript);
						// setPathTranslated
						std::cout << "SCRIPT_NAME: "<< _scriptName << std::endl;
						std::cout << "PATH_INFO: " << _pathInfo << std::endl;
						_locationObj = loc;
						return ;
					}
					std::cout << "location: " << _serverConfigs[i].locations[j].path
						<< " => " << pathParts[k] << " => " 
						<< _serverConfigs[i].locations[j].alias << std::endl;
				}
			}
		}
	}
	if (!loc && defLoc)
		_locationObj = defLoc;
	_locationObj = loc;
}

size_t	HttpRequest::posOfScriptName(std::vector<std::string> &parts, std::vector<std::string> cgiExt, size_t n)
{
	size_t idx_scrpt = 0;
	bool	found = false;
	size_t	idx_found = 0;
	while (idx_scrpt < parts.size()) {
		for (size_t i = 0; i < cgiExt.size(); i++) {
			if (parts[idx_scrpt] == cgiExt[i] && found == false) {
				found = true;
				idx_found = idx_scrpt;
			}
		}
		idx_scrpt++;
	}
	if (found)
		return idx_found + 1;
	return n;
}


bool	HttpRequest::isListeningTo(size_t i, const std::string &listeningInterface)
{
	std::cout << "HttpRequest::isListening()\n\tIP:PORT: " << listeningInterface << std::endl;
	for (size_t i_ip = 0; i_ip < _serverConfigs[i].listenInterfaces.size(); i_ip++)
	{
		std::cout << "\t" << _serverConfigs[i].listenInterfaces[i_ip] << std::endl;
		if (listeningInterface == _serverConfigs[i].listenInterfaces[i_ip])
		{
			std::cout << "isListening() ==> TRUE" << std::endl;
			return true;
		}
	}
	std::cout << "isListening() ==> FALSE" << std::endl;
	return false;
}

void	HttpRequest::setScriptName(std::vector<std::string> &parts, size_t n)
{
	for (size_t i = 1; i < n; i++)
		_scriptName += parts[i];
}

void	HttpRequest::setPathInfo(std::vector<std::string> &parts, size_t start)
{
	for (size_t i = start; i < parts.size(); i++)
		_pathInfo += parts[i];
}

/**
	* @brief splits the path at every '/' and stores in vector.
	* @param path -> the resource path.
	* @return vector of strings with the subpaths.
*/
std::vector<std::string> HttpRequest::splitPath(const std::string &path)
{
	std::cout << "HttpRequest::splitPath()" << std::endl;
	std::vector<std::string>	parts;
	std::string	temp;
	size_t	start = 0;
	size_t	end = 0;

	for (size_t i = 0; i < path.size(); i++)
	{
		if (path[i] == '/' && i == 0)
		{
			end = i+1;
			temp = path.substr(start, end - start);
			parts.push_back(temp);
			// start = end;
		}
		else if ((path[i] == '/' || path[i] == '.') && i > 0)
		{
			end = i;
			temp = path.substr(start, end - start);
			parts.push_back(temp);
			start = end;
		}
		else if (i + 1 == path.size())
		{
			end = i+1;
			temp = path.substr(start, end - start);
			parts.push_back(temp);
			break ;
		}
	}
	for (size_t i = 0; i < parts.size(); i++) // print parts
		std::cout << "\tpart[" << i << "] = " << parts[i] << std::endl;
	return parts;
}

bool	HttpRequest::isAllowedMethod()
{
	if (!_locationObj)
		return (false);
	for (size_t i = 0; i < _locationObj->allowedMethods.size(); i++) {
		if (_requestLine.method == _locationObj->allowedMethods[i])
			return true;
	}
	return false;
}


/**
	* @brief Extracts the message-body and parsses into std::string.
	* @return false, if not received full body.
	*         false & _state = PARSING_ERROR, if error occured.
	*         true, if message-body extracted successfully.
**/
bool	HttpRequest::extractBody()
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::extractBody()" << std::endl;
	std::cout << "_messageBuffer.size() = " << _messageBuffer.size() << std::endl;
    size_t contentLength = 0;

    std::map<std::string, std::vector<std::string> >::iterator it;
    it = _headers.find("content-length"); // Get Content-Length from headers
    if (it != _headers.end() && !it->second.empty())
	{
		if (_headers.find("transfer-encoding") != _headers.end()) {
			_state = PARSING_ERROR;
			return setErrorCode(400);
		}
		std::cout << "\tCONTENT-LENGTH HANDLING " << it->second[0] << std::endl;
		if (it->second.size() > 1) {
			_state = PARSING_ERROR;
			return setErrorCode(400);
		}
		for (size_t i = 0; i < it->second[0].size(); i++) {
			if (it->second[0][i] < '0' || it ->second[0][i] > '9') {
				_state = PARSING_ERROR;
				return setErrorCode(400);
			}
		}
        contentLength = atoi(it->second[0].c_str());
        // if (contentLength > MAX_BODY_SIZE)// Check body-size limit
		if (!validBodySize(contentLength))
        {
            _state = PARSING_ERROR;
            return setErrorCode(413);
        }
    }
	else {
		std::cout << "\tIN ELSE" << std::endl;
		it = _headers.find("transfer-encoding");
		if (it != _headers.end())
		{
			if (!it->second.empty() && it->second[0] == "chunked") {
				std::cout << "=====\tCHUNKED TRANSFER HANDLING" << std::endl;
				return unchunkBody();
			}
			_state = PARSING_ERROR;
			return setErrorCode(400);
		}
	}
	// REMOVE to avoid 411 if method is not allowed!
	// if (_requestLine.method == "POST" && contentLength == 0) {
	// 	_state = PARSING_ERROR;
	// 	std::cout << "PARSING_ERROR" << std::endl;
	// 	return setErrorCode(411);
	// }
	std::cout << "content-length = " << contentLength << std::endl;
	size_t availableBytes = _messageBuffer.size() - _current_pos;
	std::cout << "availableBytes = " << availableBytes << std::endl;

	if (availableBytes < contentLength)
	{
		std::cout << availableBytes << " < " << contentLength << std::endl;
		std::cout << "FALSE" << std::endl;
        return false;
    }
    if (contentLength > 0)
    {
        // _fullMessageBody = _messageBuffer.substr(_current_pos, contentLength);// Extract body
        _fullMessageBody = _messageBuffer.substr(_current_pos);// Extract body
		// std::cout << _fullMessageBody << std::endl;
        _current_pos += contentLength;
    }
    else
        _fullMessageBody = "";
    return true;
}

/**
 * @brief no chunk-extension yet.
 */
bool	HttpRequest::unchunkBody()
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::unchunkBody()" << std::endl;	
	size_t	posEnd = _messageBuffer.find("\r\n", _current_pos);
	if (posEnd == std::string::npos) {
		std::cout << "\tposEnd == std::string::npos" << std::endl;
		return false;
	}
	bool	done = false;
	while (done == false) {
		size_t	pos_cpy = _current_pos;
		size_t	chunked_size = chunkedSize();
		std::cout << "\tchunked_size = " << chunked_size << std::endl;
		if (chunked_size == 0) {
			done = true;
			std::cout << "unchunking done..." << std::endl;
			break ;
		}
		// Return false if incomplete chunk header
		if (chunked_size == (size_t)-1)	 // signal for incomplete data
		{
			std::cout << "\tIncomplete chunk header" << std::endl;
			return false;
		}
		std::string	chunked_data = chunkedData(chunked_size);

		// If empty string AND size != 0, not enough data yet
		if (chunked_data.empty() && chunked_size != 0)
		{
			_current_pos = pos_cpy;
			std::cout << "\tNot enough data yet." << std::endl;
			return false; // wait for more data from network
		}
		if (chunked_data.size() > MAX_BODY_SIZE) { // total size check
			_state = PARSING_ERROR;
			return setErrorCode(413);
		}

		_fullMessageBody += chunked_data;
		// std::cout << "body = " << _fullMessageBody << std::endl;
	}
	std::cout << "\n=====\tEND unchunkBody()\t=====" << std::endl;
	return true;
}

std::string	HttpRequest::chunkedData(size_t chunked_size)
{
	if (PRINT_REQUEST)
		std::cout << "\tHttpRequest::unchunkData()" << std::endl;

	// Check if we have enough bytes for the data + trailing CRLF
	if (_current_pos + chunked_size + 2 > _messageBuffer.size())
		return ""; // Not enough data yet, signal to wait

	size_t posEndData = _messageBuffer.find("\r\n", _current_pos);
	if (posEndData == std::string::npos) {
		std::cout << "\tNO CRLF!" << std::endl;
		return ""; // CRLF not found, need more data
	}

	std::string data = _messageBuffer.substr(_current_pos, chunked_size);
	_current_pos = posEndData + 2;
	return data;
}

size_t	HttpRequest::chunkedSize()
{
	if (PRINT_REQUEST)
		std::cout << "\tHttpRequest::chunkedSize()" << std::endl;

	size_t	posEndSize = _messageBuffer.find("\r\n", _current_pos);
	if (posEndSize == std::string::npos)
		return (size_t)-1; // Signal: incomplete chunk header, need more data

	std::string sizeStr = _messageBuffer.substr(_current_pos, posEndSize - _current_pos);
	for (size_t i = 0; i < sizeStr.size(); i++) {
		if ((sizeStr[i] < '0' || sizeStr[i] > '9') && (sizeStr[i]< 'a' || sizeStr[i] > 'f'))
		{
			_state = PARSING_ERROR;
			setErrorCode(400);
			return 0;
		}
	}
	size_t chunked_size = strtol(sizeStr.c_str(), NULL, 16);

	_current_pos = posEndSize + 2;
	return chunked_size;
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
// TODO empty content-type, but body is there
// "application/octet-stream"
std::string HttpRequest::parseContentType(std::vector<std::string> value)
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::parseContentType" << std::endl;
	std::string	temp;
	std::string	parameter;
	std::string type;
	size_t 		posSemiColon = 0;
	size_t 		posSlash = 0;

	if (value.empty())
		return type;
	temp = value.at(0);
	std::cout << "temp" << std::endl;
	if ((posSlash = temp.find('/', 0)) < temp.size())
	{
		_contentData.type = temp.substr(0, posSlash);
		if ((posSemiColon = temp.find(';', posSlash)) < temp.size())
		{
			parameter = temp.substr(posSemiColon, temp.size() - posSemiColon);
			_contentData.subtype = temp.substr(posSlash + 1, posSemiColon - posSlash - 1);
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
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::createBodyParser()" << std::endl;
	std::map<std::string, std::vector<std::string> >::iterator it;
	it = _headers.find("content-type");
	if (it == _headers.end()) {
		if (_fullMessageBody.size() > 0) { // application/octet-stream
			_contentData.type = "application";
			_contentData.subtype = "octet-stream";
			return false;
		}
		std::cerr << "\tErr: content-type is not found!" << std::endl;
		return false;
	}
	parseContentType(_headers["content-type"]);
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
	else
		return false;
	// else if (_contentData.type.size() > 0)
		// return setErrorCode(405); // was 400 but for tester -> 405
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
		|| _requestLine.method == "DELETE"
		|| _requestLine.method == "HEAD")
		return true;
	return false;
}

bool HttpRequest::isHttpVersionSupported() // 505
{
	if (_requestLine.version == "HTTP/1.1" || _requestLine.version == "HTTP/1.0")
		return true;
	return false;
}

bool HttpRequest::validRequest()
{
	std::cout << "HttpRequest::validRequest()" << std::endl;
	if (_errorCode != 0)
		return false;
	if (!isImplementedMethod())
	{
		_errorCode = 405; // actually 501 Not implemented???
		return false;
	}
	if (!isHttpVersionSupported())
	{
		_errorCode = 505;
		return false;
	}
	if (!isValidURI(_requestLine.requestURI))
	{
		_errorCode = 403;
		return false;
	}
	// std::cout << 1 << std::endl;
    std::map<std::string, std::vector<std::string> >::iterator it;
    it = _headers.find("host");
	if (it == _headers.end() || it->second.size() == 0)
	{
		_errorCode = 400;
		return false;
	}
	else {
		_host = it->second[0];
		for (size_t i = 0; i < _host.size(); i++) {
			if (!isalnum(_host[i]) && _host[i] != '.' && _host[i] != '/' && _host[i] != ':')
			{
				_errorCode = 400;
				return false;
			}
		}
		size_t posSemi;
		posSemi = _host.find_last_of(':');
		if (posSemi != std::string::npos) {
			for (size_t i = posSemi + 1; i < _host.size(); i++) {
				if (_host[i] < '0' || _host[i] > '9')
				{
					_errorCode = 400;
					return false;
				}
			}
		}
	}
//	REMOVE to avoid 411 if not allowed Method
	if (_requestLine.method == "POST")
	{
		it = _headers.find("content-length");
		if (it != _headers.end() && it->second[0] == "0" && _fullMessageBody.size() == 0) {
			_errorCode = 200;
			return false;
		}
		it = _headers.find("content-type");
		if (it != _headers.end() && _fullMessageBody.size() == 0) {
			_errorCode = 400;
			return false;
		}
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

/**
	* @brief checks for path traversal (../../etc/passwd)
*/
bool	HttpRequest::isValidURI(const std::string &uri)
{
	if (uri.find("..") != std::string::npos
		|| uri.find("//") != std::string::npos)
		return false;
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
	std::cout << "====================" << std::endl;
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
	// std::cout.write(_fullMessageBody.c_str(), 100);
	if (_fullMessageBody.size() < 100)
		std::cout << _fullMessageBody;
	std::cout << "\n}" << std::endl;
	std::cout << "====================" << std::endl;
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

void	HttpRequest::extractFileExtension()
{
	size_t	posExt = _requestLine.requestURI.find('.');
	if (posExt != std::string::npos)
	{
		size_t	i = 0;
		while (isalpha(_requestLine.requestURI[1 + posExt + i]))
			i++;
		_fileExtension = _requestLine.requestURI.substr(posExt, i + 1);
		std::cout << "\t_fileExtension: "<< _fileExtension << std::endl;
	}
}

/**
 * @brief Checks for Query char ('?') & sets position of it.
 */
bool	HttpRequest::hasQuery(size_t *posQuery)
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::hasQuery()" << std::endl;

	size_t	pos = 0;
	pos = _requestLine.requestURI.find('?');
	if (pos != std::string::npos)
	{
		*posQuery = pos;
		std::cout << "\tpos of '?' = " << pos << std::endl;
		return true;
	}
	return false;
}

bool	HttpRequest::validBodySize(size_t contentLength)
{
	std::cout << "validBodySize -> contentLength: " << contentLength << std::endl;
	if (contentLength > MAX_BODY_SIZE)
		return false;
	if (_locationObj && _locationObj->sizeIsSet
		&& contentLength > _locationObj->maxBodySize)
		return false;
	// also check servers max_body_size.
	return true;
}

/**
 * @brief Splits URI + Query and sets new request-URI.
 */
void	HttpRequest::handleQuery(size_t posQuery)
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::handleQuery()" << std::endl;

	std::string	uri = _requestLine.requestURI;
	_requestLine.requestURI = uri.substr(0, posQuery);
	std::cout << "\tnew URI = " << _requestLine.requestURI << std::endl;
	std::string	queryStr = uri.substr(posQuery + 1);
	_requestLine.queryStr = queryStr;
	if (queryStr.size() > MAX_QUERY_STRING_LENGTH)
	{
		_state = PARSING_ERROR;
		setErrorCode(414);
		return;
	}
	setQueryPairs(queryStr);
}

/**
 * @brief split key=value&key=value&
 */
void	HttpRequest::setQueryPairs(const std::string &queryStr)
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::setQueryPairs()" 
			<< "\n\tQuery = " << queryStr << std::endl;

	size_t	start = 0;
	size_t	end = 0;
	while (end < queryStr.size())
	{
		end = queryStr.find('&', start);
		if (end == std::string::npos)
			end = queryStr.size();
		// std::cout << "\t:end = " << end << std::endl;
		size_t posEqual = queryStr.find('=', start);
		// std::cout << "\t:posEqual = " << posEqual << std::endl;
		if (posEqual != std::string::npos)
		{
			setQueryKeyValue(queryStr, start, posEqual, end);
		}
		start = ++end;
	}

// Print 
	if (PRINT_REQUEST)
	{
		std::cout << "QUERY-Key-Value-Pairs:" << std::endl;
		for (size_t i = 0; i < _requestLine.query.size(); i++)
		{
			std::cout << "\t(\"" << _requestLine.query[i].key 
				<< "\"=\"" << _requestLine.query[i].value << "\")" 
				<< std::endl;
		}
	}
}

/**
 * @brief
 */
void	HttpRequest::setQueryKeyValue(const std::string &queryStr, size_t start, size_t posEqual, size_t end)
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::setQueryKeyValue()" << std::endl;

	t_query	query;

	query.key = queryStr.substr(start, posEqual - start);
	query.value = queryStr.substr(posEqual + 1, end - posEqual - 1);
	// std::cout << "\t("<< query.key << " = " << query.value << ")" << std::endl;
	_requestLine.query.push_back(query);
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

std::string &HttpRequest::getHost()
{
	return _host;
}


std::string	HttpRequest::getFileExtension()
{
	return (_fileExtension);
}

std::string	&HttpRequest::getPathInfo()
{
	return _pathInfo;
}

std::string &HttpRequest::getScriptName()
{
	return _scriptName;
}

std::string &HttpRequest::getPathTranslated()
{
	return _pathTranslated;
}

t_Location	*HttpRequest::getLocationObj()
{
	return _locationObj;
}

void	HttpRequest::setServerConfigs(std::vector<t_Configs> serverConfigs, const std::string &listeningInterface)
{
	_serverConfigs = serverConfigs;
	_listeningInterface = listeningInterface;
}

bool	HttpRequest::hasBodyContentLength()
{
	if (_fullMessageBody.size() > 0)
		return true;
	return false;
}
