#include "RequestLineParser.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================
RequestLineParser::RequestLineParser()
{
}

RequestLineParser::RequestLineParser(t_RequestData *data_)
{
	data = data_;
}

RequestLineParser::~RequestLineParser()
{
}

// =========================================================================
// Public Methods
// =========================================================================

bool	RequestLineParser::parseRequestLine()
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::parseRequestLine()" << std::endl;
	skipEmptyLines();
    size_t posCRLF = data->_messageBuffer.find("\r\n", data->_current_pos);
	if (posCRLF == std::string::npos)
		return checkForCRandLF();
	std::string reqLine = data->_messageBuffer.substr(data->_current_pos, posCRLF - data->_current_pos);
    if (reqLine.size() > MAX_REQUEST_LINE_LENGTH)
		return setErrorCode(414);
    size_t posSP1 = reqLine.find(' ');
    size_t posSP2 = reqLine.find_last_of(' ');
    if (posSP1 == posSP2 || posSP1 == std::string::npos || posSP2 == std::string::npos)
		return setErrorCode(400);
	if (!setRequestLineParts(reqLine, posSP1, posSP2))
		return setErrorCode(400);
	if (data->_requestLine.requestURI.size() > MAX_URI_LENGTH)
		setErrorCode(414);
	handleQuery();
	if (!validURI() || !validMethod() || !validHttpVersion() || !decodeURI())
		return setErrorCode(400);
	extractFileExtension();
	data->_current_pos = posCRLF + 2;  // Skip \r\n
	if (foundEndOfRequest()) {
		data->_state = PARSING_COMPLETE;
		return false;
	}
	return true;
}

// =========================================================================
// Private Methods
// =========================================================================

bool	RequestLineParser::validHttpVersion()
{
	size_t pos_version = data->_requestLine.version.find("HTTP/");
	if (pos_version == std::string::npos) 
		return false;
	else if ((data->_requestLine.version[5] < '0' || data->_requestLine.version[5] > '9')
			|| (data->_requestLine.version[7] < '0' || data->_requestLine.version[7] > '9' )
			|| (data->_requestLine.version[6] != '.'))
		return false;
	return true;
}

/**
	@brief
*/
bool	RequestLineParser::validMethod()
{
	for (size_t i = 0; i < data->_requestLine.method.size(); i++) {
		if (data->_requestLine.method[i] < 'A' || data->_requestLine.method[i] > 'Z')
			return false;
	}
	return true;
}

bool	RequestLineParser::validURI()
{
	if (!validURIstr(data->_requestLine.requestURI))
		return setErrorCode(400);
	size_t pos = 0;
	pos = data->_requestLine.requestURI.find("//");
	if (pos != std::string::npos)
		return setErrorCode(400);
	pos = data->_requestLine.requestURI.find("..");
	if (pos != std::string::npos)
		return setErrorCode(400);
	return true;
}

bool RequestLineParser::decodeURI()
{
	size_t posPercent = data->_requestLine.requestURI.find('%');
	if (posPercent == std::string::npos)
		return true;
	else {
		size_t pos = 0;
		std::string newUri;
		while ((posPercent = data->_requestLine.requestURI.find('%', pos)) != std::string::npos) {
			newUri += data->_requestLine.requestURI.substr(pos, posPercent - pos);
			std::string hex = data->_requestLine.requestURI.substr(posPercent + 1, 2);
			char c = (char) std::strtol(hex.c_str(), NULL, 16);
			if (c < 32 || c > 127) // valid character in URI?
				return false;
			newUri += c;
			pos = posPercent + 3;
		}
		newUri += data->_requestLine.requestURI.substr(pos);
		data->_requestLine.requestURI = newUri;
	}
	return true;
}

bool	RequestLineParser::setRequestLineParts(const std::string &reqLine, size_t posSP1, size_t posSP2)
{
	data->_requestLine.method = reqLine.substr(0, posSP1);
	data->_requestLine.requestURI = reqLine.substr(posSP1 + 1, posSP2 - posSP1 - 1);
	data->_requestLine.version = reqLine.substr(posSP2 + 1);
	if (data->_requestLine.method.size() < 1
		|| data->_requestLine.requestURI.size() < 1
		|| data->_requestLine.version.size() < 1) {
		data->_state = PARSING_ERROR;
		return setErrorCode(400);
	}
	return true;
}

/**
 * @brief sets PARSING_ERROR if CR or LF occured alone.
 */
bool	RequestLineParser::checkForCRandLF()
{
	size_t posLF = data->_messageBuffer.find("\n", data->_current_pos);
	if (posLF != std::string::npos)
		return setErrorCode(400);

	size_t posCR = data->_messageBuffer.find("\r", data->_current_pos);
	if (posCR != std::string::npos && posCR != data->_messageBuffer.size() -1)
		return setErrorCode(400);
	return false;
}

/**
 * @brief skips empty lines before request-line
 */
void	RequestLineParser::skipEmptyLines()
{
    while (data->_current_pos + 1 < data->_messageBuffer.size() &&
           data->_messageBuffer[data->_current_pos] == '\r' &&
           data->_messageBuffer[data->_current_pos + 1] == '\n')
        data->_current_pos += 2;
}

/**
 * @brief Splits URI + Query and sets new request-URI.
 */
void	RequestLineParser::handleQuery()
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::handleQuery()" << std::endl;

	size_t posQuery = data->_requestLine.requestURI.find('?');
	if (posQuery == std::string::npos)
		return ;
	std::string	uri = data->_requestLine.requestURI;
	data->_requestLine.requestURI = uri.substr(0, posQuery);
	std::string	queryStr = uri.substr(posQuery + 1);
	data->_requestLine.queryStr = queryStr;
	if (queryStr.size() > MAX_QUERY_STRING_LENGTH) {
		data->_state = PARSING_ERROR;
		setErrorCode(414);
		return;
	}
	setQueryPairs(queryStr);
}

/**
 * @brief split key=value&key=value&
 */
void	RequestLineParser::setQueryPairs(const std::string &queryStr)
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
		size_t posEqual = queryStr.find('=', start);
		if (posEqual != std::string::npos)
			setQueryKeyValue(queryStr, start, posEqual, end);
		start = ++end;
	}
	// Print
	if (PRINT_REQUEST) {
		std::cout << "QUERY-Key-Value-Pairs:" << std::endl;
		for (size_t i = 0; i < data->_requestLine.query.size(); i++)
		{
			std::cout << "\t(\"" << data->_requestLine.query[i].key
				<< "\"=\"" << data->_requestLine.query[i].value << "\")"
				<< std::endl;
		}
	}
}

void	RequestLineParser::extractFileExtension()
{
	size_t	posExt = data->_requestLine.requestURI.find('.');
	if (posExt != std::string::npos)
	{
		size_t	i = 0;
		while (isalpha(data->_requestLine.requestURI[1 + posExt + i]))
			i++;
		data->_fileExtension = data->_requestLine.requestURI.substr(posExt, i + 1);
		std::cout << "\t_fileExtension: "<< data->_fileExtension << std::endl;
	}
}

bool RequestLineParser::validURIchar(char c)
{
	if ((c < 'A' || c > 'Z')
			&& (c < 'a' || c > 'z')
			&& (c < '0' || c > '9')
			&& (c != '/' && c != '.' && c != '_' && c != '%' && c != '-'))
		return false;
	return true;
}

bool RequestLineParser::validURIstr(std::string &URI)
{
	for (size_t i = 0; i < URI.size(); i++) {
		if (!validURIchar(URI[i]))
			return false;
	}
	return true;
}

bool RequestLineParser::setErrorCode(int code)
{
	data->_errorCode = code;
	data->_state = PARSING_ERROR;
	return false;
}

bool RequestLineParser::foundEndOfRequest()
{
	size_t end = 0;

	end = data->_messageBuffer.find("\r\n\r\n", 0);
	if (end != std::string::npos) {
		if (end == data->_current_pos - 2)
			return true;
	}
	return false;
}

/**
 * @brief
 */
void	RequestLineParser::setQueryKeyValue(const std::string &queryStr, size_t start, size_t posEqual, size_t end)
{
	if (PRINT_REQUEST)
		std::cout << "HttpRequest::setQueryKeyValue()" << std::endl;

	t_query	query;
	query.key = queryStr.substr(start, posEqual - start);
	query.value = queryStr.substr(posEqual + 1, end - posEqual - 1);
	data->_requestLine.query.push_back(query);
}