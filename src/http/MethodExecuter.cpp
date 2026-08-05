#include "MethodExecuter.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

MethodExecuter::MethodExecuter()
{}

MethodExecuter::~MethodExecuter()
{}

// =========================================================================
// Public Methods
// =========================================================================

/**
	* @brief takes parsed Request and executes the Method, then extracts the
	*		result.
	* @param method the requested method.
	* @param request the parsed request.
	* @return t_executionResult result.
	**/
t_executionResult MethodExecuter::execute(AMethod *method, HttpRequest *request)
{
	t_executionResult	result;
	std::string			modifiedURI;

	modifiedURI = request->getRequestLine().requestURI;
	method->setRequiredData(request, modifiedURI);

	result.success = method->execute(); // execution
	result.statusCode = method->getCode();
	result.statusPhrase = method->getPhrase();
	result.body = method->getBody();
	result.uploadedLocation = method->getUploadLocation();
	if (method->isDirList())
		result.contentType = "text/html";
	else
		result.contentType = method->getContentType();
	result.lastModified = method->getLastModified();
	result.etag = method->getEtag();
	result.isCGI = false;
	std::stringstream cgiCodeStream;
	cgiCodeStream << HttpStatus::IS_CGI;
	if (result.statusCode == cgiCodeStream.str()) {
		// AMethod::executeCGI() uses IS_CGI as an internal marker (not a real
		// HTTP status) and stashes the script path in statusPhrase; translate
		// that here so no pseudo-status code leaks past this point.
		result.isCGI = true;
		result.cgiScriptPath = result.statusPhrase;
		result.statusCode = "200";
		result.statusPhrase = "OK";
	}
	return result;
}

/**
	* @brief copies server configuration and extracts rooted directories
	*	into a map.
	* @param serverConfig data structure of parsed config-file.
	* @return true.
 */
bool	MethodExecuter::setConfig(std::vector<t_Configs> serverConfigs)
{
	_serverConfigs = serverConfigs;
	for (size_t i = 0; i < _serverConfigs.size(); i++) {
		for (size_t i_ip = 0; i_ip < _serverConfigs[i].listenInterfaces.size(); i_ip++) {
			std::map<std::string, std::string> tmp_locs;
			for (size_t i_loc = 0; i_loc < _serverConfigs[i].locations.size(); i_loc++) {
				if (_serverConfigs[i].locations[i_loc].alias.size() > 0)
					tmp_locs[_serverConfigs[i].locations[i_loc].path] = _serverConfigs[i].locations[i_loc].alias;
				if (_serverConfigs[i].locations[i_loc].uploadStore.size() > 0)
					tmp_locs[_serverConfigs[i].locations[i_loc].path] = _serverConfigs[i].locations[i_loc].uploadStore;
			}
			_rootedLocs[_serverConfigs[i].listenInterfaces[i_ip]] = tmp_locs;
		}
	}
	setDefaultLocation();
	return true;
}

/**
	* @brief Modifies the request-URI with the rooted directories.
*/
std::string	MethodExecuter::modifyRequestURI(HttpRequest *req, const std::string &listeningInterface)
{
	std::string					uri;
	std::vector<std::string>	uriParts;

	uriParts = splitPath(req->getRequestLine().requestURI);
	std::map<std::string, std::map<std::string, std::string> >::iterator it_host = _rootedLocs.find(listeningInterface);
	if (it_host != _rootedLocs.end()) {
		for (size_t i = 0; i < uriParts.size(); i++) {
			std::map<std::string, std::string>::iterator it = it_host->second.find(uriParts[i]);
			if (it != it_host->second.end()) {
				if (i == 0 && uriParts.size() > 1) {
					if (it->second != uriParts[i+1]) {
						uriParts[i] = it->second;
					}
				}
				else {
					uriParts[i] = it->second;
				}
			}
		}
	}
	for (size_t i = 0; i < uriParts.size(); i++) {
		if (uriParts[i] == "/")
			continue;
		uri += uriParts[i];
	}
	return uri;
}

/**
	* @brief Checks if requested Method is implemented on server.
	* @return true (if valid method), false (not implemented).
**/
bool	MethodExecuter::isImplementedMethod(const std::string &methodName)
{
	if (methodName == "GET"
		|| methodName == "POST"
		|| methodName == "DELETE"
		|| methodName == "HEAD")
		return true;
	return false;
}

/**
	* @brief Checks if the requested-method is allowed to be executed on the 
	*	requested resource.
	* @param location -> configurations of the resource
	* @param method -> requested-Method.
	* @return true (if requested-Method is allowed), 
		false (requested-method not allowed).
*/
bool	MethodExecuter::isAllowedMethod(t_Location *location, const std::string &method)
{
	if (location->redirect)
		return true;
	for (size_t i = 0; i < location->allowedMethods.size(); i++) {
		if (method == location->allowedMethods[i]) {
			return true;
		}
	}
	return false;
}

/**
	* @brief Creates and allocates requested Method Object.
	* @param methodName -> requestedMethod.
	* @param path -> requested resource.
	* @return Pointer to AMethod-Obj, or NULL.
**/
AMethod	*MethodExecuter::createMethod(const std::string &methodName, t_Location *locationObj)
{
	AMethod		*tempMethod = NULL;
	t_Location	*location = NULL;

	location = locationObj;
	if (!location) {
		location = &_defaultLocation;
	}
	if (location != NULL) {
		if (isAllowedMethod(location, methodName)) {
			if (methodName == "GET")
				tempMethod = createGet(methodName, location);
			if (methodName == "DELETE")	
				tempMethod = createDelete(methodName, location);
			if (methodName == "POST")
				tempMethod = createPost(methodName, location);
			if (methodName == "HEAD")
				tempMethod = createHead(methodName, location);
			return tempMethod;
		}
		else
			return NULL;
	}
	if (location != NULL && !isAllowedMethod(location, methodName))
		return NULL;
	if (methodName == "GET")
		tempMethod = createGet(methodName);
	else if (methodName == "POST")
		tempMethod = createPost(methodName);
	else if (methodName == "DELETE")
		tempMethod = createDelete(methodName);
	return tempMethod;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

bool MethodExecuter::isListening(size_t i, const std::string &listeningInterface)
{
	for (size_t i_ip = 0; i_ip < _serverConfigs[i].listenInterfaces.size(); i_ip++) {
		if (listeningInterface == _serverConfigs[i].listenInterfaces[i_ip])
			return true;
	}
	return false;
}

/**
	* @brief splits the path into subpaths and searches for a 
	*	fitting t_Location struct.
	* @returns most fitting location struct.
*/
t_Location	*MethodExecuter::availableLocation(
	const std::string &path, 
	const std::string &listeningInterface, 
	const std::string &methodName)
{
	t_Location					*loc;
	t_Location					*defLoc;
	std::vector<std::string>	pathParts;

	loc = NULL;
	defLoc = NULL;
	pathParts = splitPath(path);
	for (size_t i = 0; i < _serverConfigs.size(); i++) {
		if (!isListening(i, listeningInterface))
			continue ;
		for (size_t k = 0; k < pathParts.size(); k++) {
			for (size_t j = 0; j < _serverConfigs[i].locations.size(); j++) {	
				if (!defLoc && _serverConfigs[i].locations[j].path == "/")
					defLoc = &_serverConfigs[i].locations[j];
				if (_serverConfigs[i].locations[j].path == pathParts[k]) {
					if (_serverConfigs[i].locations[j].cgi) { 
						if (isAllowedMethod(&_serverConfigs[i].locations[j], methodName))
							loc = &_serverConfigs[i].locations[j];
					}
					else
						loc = &_serverConfigs[i].locations[j];
				}
			}
		}
	}
	if (!loc && defLoc)
		loc = defLoc;
	return loc;
}

/**
	* @brief splits the path at every '/' and stores in vector.
	* @param path -> the resource path.
	* @return vector of strings with the subpaths.
*/
std::vector<std::string> MethodExecuter::splitPath(const std::string &path)
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
		else if ((path[i] == '/' || path[i] == '.') && i > 0) {
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

std::vector<std::string> MethodExecuter::splitPathDir(const std::string &path)
{
	std::vector<std::string>	parts;
	std::string					temp;
	size_t						start = 0;
	size_t						end = 0;

	for (size_t i = 0; i < path.size(); i++) {
		if (path[i] == '/' && i > 0) {
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

void	MethodExecuter::setDefaultLocation()
{
	_defaultLocation.autoIndex = false;
	_defaultLocation.redirect = false;
	_defaultLocation.cgi = false;
	_defaultLocation.sizeIsSet = false;
	_defaultLocation.upload = false;
	_defaultLocation.allowedMethods.push_back("GET");
}

AMethod *MethodExecuter::createGet(std::string name)
{
	return new Get(name);
}

AMethod *MethodExecuter::createGet(std::string name, t_Location *locationObj)
{
	return new Get(name, locationObj);
}

AMethod *MethodExecuter::createPost(std::string name)
{
	return new Post(name);
}

AMethod *MethodExecuter::createDelete(std::string name)
{
	return new Delete(name);
}

AMethod *MethodExecuter::createDelete(std::string name, t_Location *location)
{
	return new Delete(name, location);
}

AMethod *MethodExecuter::createPost(std::string name, t_Location *location)
{
	return new Post(name, location);
}

AMethod *MethodExecuter::createHead(std::string name)
{
	return new Head(name);
}

AMethod *MethodExecuter::createHead(std::string name, t_Location *locationObj)
{
	return new Head(name, locationObj);
}

std::vector<t_Configs>	MethodExecuter::getServerConfigs()
{
	return this->_serverConfigs;
}
