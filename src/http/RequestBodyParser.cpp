#include "RequestBodyParser.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

RequestBodyParser::RequestBodyParser()
{
	_bodyParser = NULL;
}

RequestBodyParser::RequestBodyParser(t_RequestData *data_)
{
	data = data_;
	_bodyParser = NULL;
}

RequestBodyParser::~RequestBodyParser()
{
	if (_bodyParser != NULL) {
		delete _bodyParser;
		_bodyParser = NULL;
	}
}

// =========================================================================
// Public Methods
// =========================================================================

void	RequestBodyParser::parseBody()
{
	if (createBodyParser()) {
		_bodyParser->setContentData(data->_contentData);
		_bodyParser->parse(data->_fullMessageBody);
		data->_parsedMessageBody = _bodyParser->getResult();
	}
	else {
		if (data->_fullMessageBody.size() > 0) {
			std::map<std::string, s_FormField> res;
			s_FormField	body;
			body.value = data->_fullMessageBody;
			body.filename = "a";
			res["-"] = body;
			data->_parsedMessageBody = res;
			data->_state = PARSING_COMPLETE;
		}
		data->_state = PARSING_ERROR;
	}
}

/**
	* @brief Extracts the message-body and parsses into std::string.
	* @return false, if not received full body.
	*         false & _state = PARSING_ERROR, if error occured.
	*         true, if message-body extracted successfully.
**/
bool	RequestBodyParser::extractBody()
{
    size_t contentLength = 0;
    std::map<std::string, std::vector<std::string> >::iterator it;
    it = data->_headers.find("content-length");
    if (it != data->_headers.end() && !it->second.empty())
	{
		if (data->_headers.find("transfer-encoding") != data->_headers.end())
			return setErrorCode(400);
		if (it->second.size() > 1)
			return setErrorCode(400);
		for (size_t i = 0; i < it->second[0].size(); i++) {
			if (it->second[0][i] < '0' || it ->second[0][i] > '9')
				return setErrorCode(400);
		}
        contentLength = atoi(it->second[0].c_str());
		if (!validBodySize(contentLength))
            return setErrorCode(413);
    }
	else {
		it = data->_headers.find("transfer-encoding");
		if (it != data->_headers.end()) {
			if (!it->second.empty() && it->second[0] == "chunked") {
				return unchunkBody();
			}
			return setErrorCode(400);
		}
	}
	size_t availableBytes = data->_messageBuffer.size() - data->_current_pos;
	if (availableBytes < contentLength) {
        return false;
    }
    if (contentLength > 0) {
        data->_fullMessageBody = data->_messageBuffer.substr(data->_current_pos);// Extract body
        data->_current_pos += contentLength;
    }
    else
        data->_fullMessageBody = "";
    return true;
}

// =========================================================================
// Private Methods
// =========================================================================

/**
	* @brief Checks Content-Type header field and creates the appropiate
	*  body-parser. this->_bodyParser.
**/
bool RequestBodyParser::createBodyParser()
{
	std::map<std::string, std::vector<std::string> >::iterator it;
	it = data->_headers.find("content-type");
	if (it == data->_headers.end()) {
		if (data->_fullMessageBody.size() > 0) { // application/octet-stream
			data->_contentData.type = "application";
			data->_contentData.subtype = "octet-stream";
			return false;
		}
		return false;
	}
	parseContentType(data->_headers["content-type"]);
	if (data->_contentData.type == "multipart")
		_bodyParser = createMultiParser();
	else if (data->_contentData.type == "application")
		_bodyParser = createFormParser();
	else
		return false;
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
// TODO empty content-type, but body is there
// "application/octet-stream"
std::string RequestBodyParser::parseContentType(std::vector<std::string> value)
{
	std::string	temp;
	std::string	parameter;
	std::string type;
	size_t 		posSemiColon = 0;
	size_t 		posSlash = 0;

	if (value.empty())
		return type;
	temp = value.at(0);
	if ((posSlash = temp.find('/', 0)) < temp.size()) {
		data->_contentData.type = temp.substr(0, posSlash);
		if ((posSemiColon = temp.find(';', posSlash)) < temp.size()) {
			parameter = temp.substr(posSemiColon, temp.size() - posSemiColon);
			data->_contentData.subtype = temp.substr(posSlash + 1, posSemiColon - posSlash - 1);
			size_t	posEqual = 0;
			if ((posEqual = parameter.find("=", 0)) < parameter.size())
				data->_contentData.boundary = parameter.substr(posEqual + 1, parameter.size() - posEqual);
		}
		else
			data->_contentData.subtype = temp.substr(posSlash + 1, temp.size() - posSlash);
		type = temp.substr(0, posSemiColon);
		parameter = temp.substr(posSemiColon + 1, temp.size());
	}
	return type;
}

ABodyParser *RequestBodyParser::createMultiParser()
{
	return new MultipartParser();
}

ABodyParser *RequestBodyParser::createFormParser()
{
	return new FormParser();
}

/**
 * @brief no chunk-extension yet.
 * chunked_size == (size_t) -1 signal for incomplete data
 * chunked_data.empty() && chunked_size!= 0 -> not enough data
 */
bool	RequestBodyParser::unchunkBody()
{
	size_t	posEnd = data->_messageBuffer.find("\r\n", data->_current_pos);
	if (posEnd == std::string::npos)
		return false;
	bool	done = false;
	while (done == false) {
		size_t	pos_cpy = data->_current_pos;
		size_t	chunked_size = chunkedSize();
		if (chunked_size == 0) {
			done = true;
			break ;
		}
		if (chunked_size == (size_t)-1)
			return false;
		std::string	chunked_data = chunkedData(chunked_size);
		if (chunked_data.empty() && chunked_size != 0) {
			data->_current_pos = pos_cpy;
			return false;
		}
		if (chunked_data.size() > MAX_BODY_SIZE)
			return setErrorCode(413);
		data->_fullMessageBody += chunked_data;
	}
	return true;
}

std::string	RequestBodyParser::chunkedData(size_t chunked_size)
{
	if (data->_current_pos + chunked_size + 2 > data->_messageBuffer.size())
		return "";

	size_t posEndData = data->_messageBuffer.find("\r\n", data->_current_pos);
	if (posEndData == std::string::npos)
		return "";

	std::string dataTemp = data->_messageBuffer.substr(data->_current_pos, chunked_size);
	data->_current_pos = posEndData + 2;
	return dataTemp;
}

size_t	RequestBodyParser::chunkedSize()
{
	size_t	posEndSize = data->_messageBuffer.find("\r\n", data->_current_pos);
	if (posEndSize == std::string::npos)
		return (size_t)-1;
	std::string sizeStr = data->_messageBuffer.substr(data->_current_pos, posEndSize - data->_current_pos);
	for (size_t i = 0; i < sizeStr.size(); i++) {
		if ((sizeStr[i] < '0' || sizeStr[i] > '9') 
			&& (sizeStr[i]< 'a' || sizeStr[i] > 'f')) {
			setErrorCode(400);
			return 0;
		}
	}
	size_t chunked_size = strtol(sizeStr.c_str(), NULL, 16);
	data->_current_pos = posEndSize + 2;
	return chunked_size;
}

/**
	* @brief Sets the _errorCode of this object to the arg code passed.
*/
bool RequestBodyParser::setErrorCode(int code)
{
	data->_errorCode = code;
	data->_state = PARSING_ERROR;
	return false;
}

bool	RequestBodyParser::validBodySize(size_t contentLength)
{
	if (contentLength > MAX_BODY_SIZE)
		return false;
	if (data->_locationObj && data->_locationObj->sizeIsSet
		&& contentLength > data->_locationObj->maxBodySize)
		return false;
	return true;
}