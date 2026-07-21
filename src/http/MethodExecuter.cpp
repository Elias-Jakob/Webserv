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
t_executionResult MethodExecuter::execute(AMethod *method, HttpRequest *request, const std::string &listeningInterface)
{
	std::cout << "MethodExecuter::execute()" << std::endl;
	t_executionResult	result;
	std::string			modifiedURI;

	modifiedURI = modifyRequestURI(request, listeningInterface);
	method->setRequiredData(request, modifiedURI);

	result.success = method->execute(); // execution
	// if (result.success)
	// {
		result.statusCode = method->getCode();
		result.statusPhrase = method->getPhrase();

		result.body = method->getBody();

		if (method->isDirList())
			result.contentType = "text/html";
		else
			result.contentType = method->getContentType();
		result.lastModified = method->getLastModified();
		result.etag = method->getEtag();
	// }
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
	std::cout << "\nMethodExecuter::setConfig()" << std::endl;
	_serverConfigs = serverConfigs;
	std::cout << "\n===========" << std::endl;
	for (size_t i = 0; i < _serverConfigs.size(); i++)
	{
		std::cout << "server_name = " << _serverConfigs[i].serverName << std::endl;
		std::cout << "IP:Port n = " << _serverConfigs[i].listenInterfaces.size() << std::endl;
		for (size_t i_ip = 0; i_ip < _serverConfigs[i].listenInterfaces.size(); i_ip++)
		{
			std::map<std::string, std::string> tmp_locs;
			for (size_t i_loc = 0; i_loc < _serverConfigs[i].locations.size(); i_loc++)
			{
				if (_serverConfigs[i].locations[i_loc].root.size() > 0)
					tmp_locs[_serverConfigs[i].locations[i_loc].path] = _serverConfigs[i].locations[i_loc].root;
				if (_serverConfigs[i].locations[i_loc].uploadStore.size() > 0)
					tmp_locs[_serverConfigs[i].locations[i_loc].path] = _serverConfigs[i].locations[i_loc].uploadStore;
			}
			_rootedLocs[_serverConfigs[i].listenInterfaces[i_ip]] = tmp_locs;
		}
	}

// PRINT ROOTED_LOCATIONs
	std::cout << "_rootedLocs.size() = " << _rootedLocs.size() << std::endl;
	std::map<std::string, std::map<std::string, std::string> >::iterator	it = _rootedLocs.begin();
	std::map<std::string, std::map<std::string, std::string> >::iterator	ite = _rootedLocs.end();
	while (it != ite)
	{
		std::cout << "Listen: " << it->first << "\n{"<< std::endl;
		std::map<std::string, std::string>::iterator itLocs = it->second.begin();
		std::map<std::string, std::string>::iterator iteLocs = it->second.end();
		while (itLocs != iteLocs)
		{
			std::cout << "\t" << itLocs->first << " => " << itLocs->second << std::endl;
			itLocs++;
		}
		it++;
		std::cout << "}" << std::endl;
	}
	setDefaultLocation();
	return true;
}

/**
	* @brief Modifies the request-URI with the rooted directories.
*/
std::string	MethodExecuter::modifyRequestURI(HttpRequest *req, const std::string &listeningInterface)
{
	std::cout << "MethodExecuter::modifyRequestURI()... " << req->getRequestLine().requestURI << std::endl;
	std::string					uri;
	std::vector<std::string>	uriParts;

	uriParts = splitPath(req->getRequestLine().requestURI);
	for(size_t i = 0; i < uriParts.size(); i++)
		std::cout << "\t" << i << ": ("<< uriParts[i] << ")" << std::endl;
	std::cout << "uriParts.size() = " << uriParts.size() << std::endl;
	// std::string host = req->getHost();
	std::map<std::string, std::map<std::string, std::string> >::iterator it_host = _rootedLocs.find(listeningInterface);
	if (it_host != _rootedLocs.end())
	{
		// std::map<std::string, std::string>::iterator it = _rootedLocations.begin();
		// std::map<std::string, std::string>::iterator ite = _rootedLocations.end();
		std::map<std::string, std::string>::iterator it = it_host->second.begin();
		std::map<std::string, std::string>::iterator ite = it_host->second.end();
		std::cout << "ROOTED_LOCATIONS: " << std::endl;
		while (it != ite) {
			std::cout << "\t" << it->first << " -> " << it->second << std::endl;
			it++;
		}

		for (size_t i = 0; i < uriParts.size(); i++) // lookup rooted Locations & replace if found
		{
			std::cout << "loop to replace.." << i << std::endl;
			std::map<std::string, std::string>::iterator it = it_host->second.find(uriParts[i]);
			if (it != it_host->second.end())
			{
				if (i == 0 && uriParts.size() > 1) {
					std::cout << "\t" << it->second << " -> " << uriParts[i+1] << std::endl;
					if (it->second != uriParts[i+1]) {
						uriParts[i] = it->second;
						std::cout << "\t -replace[0]: " << uriParts[i] << " <-> " << it->second << std::endl;
					}
				}
				else {
					std::cout << "\t - replace: " << uriParts[i] << " <-> " << it->second << std::endl;
					uriParts[i] = it->second;
				}
			}
		}
	}
	else
		std::cout << "no Rooted locations found..." << std::endl;
	for (size_t i = 0; i < uriParts.size(); i++) // create new uri
	{
		if (uriParts[i] == "/")
			continue;
		uri += uriParts[i];
	}
	std::cout << "MethodExecuter::modifyRequestURI() ==> " << uri << std::endl;
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
	for (size_t i = 0; i < location->allowedMethods.size(); i++)
	{
		if (method == location->allowedMethods[i])
		{
			std::cout << "MethodExecuter()::isAllowedMethod() ==> TRUE" << std::endl;
			return true;
		}
	}
	std::cout << "MethodExecuter()::isAllowedMethod() ==> FALSE" << std::endl;
	return false;
}

/**
	* @brief Creates and allocates requested Method Object.
	* @param methodName -> requestedMethod.
	* @param path -> requested resource.
	* @return Pointer to AMethod-Obj, or NULL.
**/
AMethod	*MethodExecuter::createMethod(const std::string &methodName, const std::string &path, const std::string &listeningInterface)
{
	std::cout << "MethodExecuter::createMethod() -> " << methodName << std::endl;
	AMethod		*tempMethod = NULL;
	t_Location	*location = NULL;

	location = availableLocation(path, listeningInterface, methodName); // 
	if (!location)
	{
		location = &_defaultLocation;
		std::cout << "\tSet _defaultLocation for resource" << std::endl;
	}
	if (location != NULL)
	{
		if (isAllowedMethod(location, methodName)) // Implement for all Methods
		{
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
		std::cout << "location.root = " << location->root << std::endl;
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
	std::cout << "isListening()\n\tlisteningInterface: " << listeningInterface << std::endl;
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

/**
	* @brief splits the path into subpaths and searches for a 
	*	fitting t_Location struct.
	* @returns most fitting location struct.
*/
t_Location	*MethodExecuter::availableLocation(const std::string &path, const std::string &listeningInterface, const std::string &methodName)
{
	std::cout << "\nMethodExecuter::availableLocation(), path = " << path << std::endl;
	t_Location					*loc;
	t_Location					*defLoc;
	std::vector<std::string>	pathParts;

	loc = NULL;
	defLoc = NULL;
	pathParts = splitPath(path);

// check for file-extension for an cgi_extension
	size_t	pos_ext = path.find(".bla", 0);
	if (pos_ext == std::string::npos)
		std::cout << "  Did not find file extension!" << std::endl;
	else
		std::cout << "  Found file extension!" << std::endl;
	
	std::cout << "===== LOOP =====" << std::endl;
	for (size_t i = 0; i < _serverConfigs.size(); i++)
	{
		std::cout << i << std::endl;
		if (!isListening(i, listeningInterface))
			continue ;
		for (size_t k = 0; k < pathParts.size(); k++)
		{
			for (size_t j = 0; j < _serverConfigs[i].locations.size(); j++)
			{	
				std::cout << pathParts[k] << " == " 
							<< _serverConfigs[i].locations[j].path << std::endl;
				if (!defLoc && _serverConfigs[i].locations[j].path == "/")
					defLoc = &_serverConfigs[i].locations[j];
				if (_serverConfigs[i].locations[j].path == pathParts[k])// && k == pathParts.size() - 1)
				{
					if (_serverConfigs[i].locations[j].cgi) { 
						if (isAllowedMethod(&_serverConfigs[i].locations[j], methodName)) {
							loc = &_serverConfigs[i].locations[j];
							std::cout << "set CGI path" << std::endl;
						}
					}
					else {
						loc = &_serverConfigs[i].locations[j];
						std::cout << "location: " << _serverConfigs[i].locations[j].path
							<< " => " << pathParts[k] << " => " << _serverConfigs[i].locations[j].root << std::endl;
					}
				}
			}
		}
	}
	if (!loc && defLoc)
		loc = defLoc;
	if (loc != NULL) // print result
		std::cout << "\t ==> location " << loc->path << " => " << loc->root << " {..}" << std::endl;
	else
		std::cout << "\t ==> location NULL" << std::endl;
	return loc;
}

/**
	* @brief splits the path at every '/' and stores in vector.
	* @param path -> the resource path.
	* @return vector of strings with the subpaths.
*/
std::vector<std::string> MethodExecuter::splitPath(const std::string &path)
{
	std::cout << "MethodExecuter::splitPath()" << std::endl;
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

std::vector<std::string> MethodExecuter::splitPathDir(const std::string &path)
{
	std::cout << "MethodExecuter::splitPathDir()" << std::endl;
	std::vector<std::string>	parts;
	std::string					temp;
	size_t						start = 0;
	size_t						end = 0;

	for (size_t i = 0; i < path.size(); i++)
	{
		if (path[i] == '/' && i > 0)
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
	// for (size_t i = 0; i < parts.size(); i++) // print parts
	// 	std::cout << "\tpart[" << i << "] = " << parts[i] << std::endl;
	return parts;
}

void	MethodExecuter::setDefaultLocation()
{
	_defaultLocation.autoIndex = false;
	_defaultLocation.redirect = false;
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
