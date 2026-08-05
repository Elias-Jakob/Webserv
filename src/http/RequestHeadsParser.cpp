#include "RequestHeadsParser.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

RequestHeadsParser::RequestHeadsParser()
{}

RequestHeadsParser::RequestHeadsParser(t_RequestData *data_)
{
	data = data_;
}

RequestHeadsParser::~RequestHeadsParser()
{}

// =========================================================================
// Public Methods
// =========================================================================

bool RequestHeadsParser::parseHeaderLine()
{
    size_t endOfHeaders = data->_messageBuffer.find("\r\n\r\n", data->_current_pos);
    if (endOfHeaders == std::string::npos)
        return false;
    while (data->_current_pos < endOfHeaders) {
		size_t	lineEnd;
		std::string line = extractHeader(&lineEnd);
        if (line.empty())
            break;
        if (data->_headers.size() >= MAX_HEADERS)
            return setErrorCode(431);
		if (line.size() > MAX_HEADER_LENGTH || !setHeaderPair(line))
			return setErrorCode(400);
		setCurrentPos(lineEnd + 2);
    }
	setCurrentPos(endOfHeaders + 4);

	std::vector<std::string> pathParts = splitPath(data->_requestLine.requestURI);
	findLocation(pathParts);
	modifyURI(pathParts);
	return true;
}

// =========================================================================
// Private Methods
// =========================================================================

std::string	RequestHeadsParser::extractHeader(size_t *lineEnd)
{
	*lineEnd = data->_messageBuffer.find("\r\n", data->_current_pos);
	if (*lineEnd == std::string::npos)
		return "";
	std::string line = data->_messageBuffer.substr(data->_current_pos, *lineEnd - data->_current_pos);
	return line;
}

bool	RequestHeadsParser::setHeaderPair(const std::string &line)
{
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos && colonPos != 0) {
        std::string key = line.substr(0, colonPos);
		toLowerCase(key);
		size_t valueStart = colonPos + 1;
           while (valueStart < line.size() && line[valueStart] == ' ')
               valueStart++;
		std::string value = line.substr(valueStart);
		if (!validHeaderPair(key, value))
			return false;
		addHeader(key, value);
		if (key == "host")
			data->_host = value;
		return true;
	}
	return false;
}

bool	RequestHeadsParser::validHeaderPair(const std::string &key, const std::string &value)
{
	for (size_t i = 0; i < key.size(); i++) {
		if (!isalpha(key[i]) && key[i] != '-')
			return false;
	}
	if (key.size() > MAX_HEADER_NAME_LENGTH 
		|| value.size() > MAX_HEADER_VALUE_LENGTH)
		return false;
	return true;
}

std::vector<std::string> RequestHeadsParser::splitHeaderValByComma(std::string val)
{
	std::vector<std::string>	split;
	size_t	i = val.size();
	size_t	start = 0;
	size_t	end = 0;

	while(end < i) {
		end = val.find(',', start);
		size_t spaces = skipLWS(val, start, end);
		start+= spaces;
		if (end < i)
			split.push_back(val.substr(start, end - start));
		else if (end >= i) {
			split.push_back(val.substr(start, val.size()));
			break ;
		}
		start = end + 1;
	}
	return split;
}

std::string	RequestHeadsParser::toLowerCase(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

void	RequestHeadsParser::addHeader(const std::string &key, const std::string &value)
{
	std::map<std::string, std::vector<std::string> >::iterator it;
	it = data->_headers.find(key);
	if (it == data->_headers.end())
           data->_headers[key] = splitHeaderValByComma(value);
	else {
		if (key == "host" || key == "content-length")
			setErrorCode(400);
		std::vector<std::string>	temp;
		temp = splitHeaderValByComma(value);
		for (size_t i = 0; i < temp.size(); i++)
			it->second.push_back(temp[i]);
	}
}

size_t RequestHeadsParser::skipLWS(std::string val, size_t start, size_t end)
{
	size_t cnt = 0;
	while (val[start] == ' ' && start < end) {
		cnt++;
		start++;
	}
	return (cnt);
}

void	RequestHeadsParser::adjustCurrentPos(size_t pos)
{
	data->_current_pos += pos;
}

void	RequestHeadsParser::setCurrentPos(size_t pos)
{
	data->_current_pos = pos;
}

/**
	* @brief Sets the _errorCode of this object to the arg code passed.
*/
bool	RequestHeadsParser::setErrorCode(int code)
{
	data->_errorCode = code;
	data->_state = PARSING_ERROR;
	return false;
}

/**
	* @brief splits the path at every '/' and stores in vector.
	* @param path -> the resource path.
	* @return vector of strings with the subpaths.
*/
std::vector<std::string> RequestHeadsParser::splitPath(const std::string &path)
{
	std::vector<std::string>	parts;
	std::string	temp;
	size_t	start = 0;
	size_t	end = 0;

	for (size_t i = 0; i < path.size(); i++) {
		if (path[i] == '/' && i == 0) {
			end = i+1;
			temp = path.substr(start, end - start);
			parts.push_back(temp);
		}
		else if ((path[i] == '/') && i > 0) {
			end = i;
			temp = path.substr(start, end - start);
			parts.push_back(temp);
			start = end;
		}
		else if (i + 1 == path.size()) {
			end = i+1;
			temp = path.substr(start, end - start);
			parts.push_back(temp);
			break ;
		}
	}
	return parts;
}

// in MethodExecuter, we us _rootedLocs for this
void	RequestHeadsParser::modifyURI(std::vector<std::string> &pathParts)
{
	if (!data->_locationObj->cgi) {
		size_t	idx_server;
		for (idx_server = 0; idx_server < data->_serverConfigs.size(); idx_server++) {
			if (isListeningTo(idx_server, data->_listeningInterface))
				break ;
		}
		std::string newURI;
		if (data->_serverConfigs[idx_server].root.size() > 0)
			newURI = data->_serverConfigs[idx_server].root;
		for (size_t iUri = 1; iUri < pathParts.size(); iUri++) {
			if (pathParts[iUri] == data->_serverConfigs[idx_server].root)
				continue ;
			std::string part = pathParts[iUri];
			for (size_t iLocs = 0; iLocs < data->_serverConfigs[idx_server].locations.size(); iLocs++) {
				if (pathParts[iUri] == data->_serverConfigs[idx_server].locations[iLocs].path) {
					if (data->_serverConfigs[idx_server].locations[iLocs].upload)
						part = data->_serverConfigs[idx_server].locations[iLocs].uploadStore;
					else if (data->_serverConfigs[idx_server].locations[iLocs].alias.size() > 0)
						part = data->_serverConfigs[idx_server].locations[iLocs].alias;
					else if (data->_serverConfigs[idx_server].locations[iLocs].redirect) {
						newURI = data->_serverConfigs[idx_server].locations[iLocs].redirectURL;
						part = "";
						break ;
					}
				}
			}
			newURI += part;
		}
		if (data->_locationObj->formSubmit)
			newURI += "/" + data->_locationObj->formUploadFile;
		data->_requestLine.requestURI = newURI;
	}
	else if (data->_locationObj->cgi)
		modifyURIforCGI();
}

void RequestHeadsParser::modifyURIforCGI()
{
	std::vector<std::string> pathParts = splitPath(data->_requestLine.requestURI);
	size_t	idx_server;
	for (idx_server = 0; idx_server < data->_serverConfigs.size(); idx_server++) {
		if (isListeningTo(idx_server, data->_listeningInterface))
			break ;
	}
	std::string newURI;
	if (data->_serverConfigs[idx_server].root.size() > 0)
		newURI = data->_serverConfigs[idx_server].root;
	for (size_t iUri = 1; iUri < pathParts.size(); iUri++) {
		std::string part = pathParts[iUri];
		for (size_t iLocs = 0; iLocs < data->_serverConfigs[idx_server].locations.size(); iLocs++) {
			if (pathParts[iUri] == data->_serverConfigs[idx_server].locations[iLocs].path)
				part = data->_serverConfigs[idx_server].locations[iLocs].alias;
		}
		newURI += part;
	}
	data->_requestLine.requestURI = newURI;
	if (data->_locationObj->cgiPath.size() > 1)
		data->_requestLine.requestURI = data->_locationObj->cgiPath;
	data->_scriptName = data->_requestLine.requestURI;
	pathParts = splitPath(data->_pathInfo);
	std::string	newPathInfo;
	for (size_t iUri = 1; iUri < pathParts.size(); iUri++) {
		std::string part = pathParts[iUri];
		for (size_t iLocs = 0; iLocs < data->_serverConfigs[idx_server].locations.size(); iLocs++) {
			if (pathParts[iUri] == data->_serverConfigs[idx_server].locations[iLocs].path)
				part = data->_serverConfigs[idx_server].locations[iLocs].alias;
		}
		newPathInfo += part;
	}
	data->_pathInfo = newPathInfo;
	data->_pathTranslated = data->_serverConfigs[idx_server].root + data->_pathInfo;
}

void	RequestHeadsParser::findLocation(std::vector<std::string> pathParts)
{
	t_Location	*loc = NULL;
	t_Location	*defLoc = NULL;
	bool		isCGI = false;

	for (size_t i = 0; i < data->_serverConfigs.size(); i++) {
		if (!isListeningTo(i, data->_listeningInterface))
			continue ;
		for (size_t k = 0; k < pathParts.size(); k++) {
			for (size_t j = 0; j < data->_serverConfigs[i].locations.size(); j++) {
				if (!defLoc && data->_serverConfigs[i].locations[j].path == "/")
					defLoc = &data->_serverConfigs[i].locations[j];
				if (data->_serverConfigs[i].locations[j].path == pathParts[k]) {
					loc = &data->_serverConfigs[i].locations[j];
					if (loc && loc->cgi && isCGI == false) {
						isCGI = true;
						size_t posScript = posOfScriptName(pathParts, loc->cgiExtensions, k + 1);
						setScriptName(pathParts, posScript);
						setPathInfo(pathParts, posScript);
						data->_locationObj = loc;
						return ;
					}
				}
			}
		}
	}
	if (!loc && defLoc)
		data->_locationObj = defLoc;
	data->_locationObj = loc;
}

size_t	RequestHeadsParser::posOfScriptName(std::vector<std::string> &parts, std::vector<std::string> cgiExt, size_t n)
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


bool	RequestHeadsParser::isListeningTo(size_t i, const std::string &listeningInterface)
{
	for (size_t i_ip = 0; i_ip < data->_serverConfigs[i].listenInterfaces.size(); i_ip++) {
		if (listeningInterface == data->_serverConfigs[i].listenInterfaces[i_ip])
			return true;
	}
	return false;
}

void	RequestHeadsParser::setScriptName(std::vector<std::string> &parts, size_t n)
{
	for (size_t i = 1; i < n; i++)
		data->_scriptName += parts[i];
}

void	RequestHeadsParser::setPathInfo(std::vector<std::string> &parts, size_t start)
{
	for (size_t i = start; i < parts.size(); i++)
		data->_pathInfo += parts[i];
}