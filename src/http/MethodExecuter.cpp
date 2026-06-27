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
	std::cout << "MethodExecuter::execute()" << std::endl;
	t_executionResult	result;
	std::string			modifiedURI;

	modifiedURI = modifyRequestURI(request);
	method->setRequiredData(request, modifiedURI);

	result.success = method->execute(); // execution
	// if (result.success)
	// {
		result.statusCode = method->getCode();
		result.statusPhrase = method->getPhrase();
		result.body = method->getBody();
		result.contentType = method->getContentType();
	// }
	return result;
}

/**
	* @brief copies server configuration and extracts rooted directories
	*	into a map.
	* @param serverConfig data structure of parsed config-file.
	* @return true.
 */
bool	MethodExecuter::setConfig(t_Server *serverConfig)
{
	std::cout << "\nMethodExecuter::setConfig()" << std::endl;
	_serverConfig = serverConfig;
	for (size_t i = 0; i < _serverConfig->locations.size(); i++)
	{
		if (_serverConfig->locations[i].root.size() > 0)
			_rootedLocations[_serverConfig->locations[i].path] = _serverConfig->locations[i].root;
	}
	std::map<std::string, std::string>::iterator it = _rootedLocations.begin();
	std::map<std::string, std::string>::iterator ite = _rootedLocations.end();
	std::cout << "_rootedLocations{\n";
	while (it != ite)
	{
		std::cout << "\t[" << it->first << "] = " << it->second << std::endl; 
		it++;
	}
	std::cout << "}" << std::endl;
	std::cout << "MethodExecuter::setConfig(): server_name = "  << _serverConfig->serverName << std::endl;
	return true;
}

/**
	* @brief Modifies the request-URI with the rooted directories.
*/
std::string	MethodExecuter::modifyRequestURI(HttpRequest *req)
{
	std::cout << "MethodExecuter::modifyRequestURI()... " << req->getRequestLine().requestURI << std::endl;
	std::string					uri;
	std::vector<std::string>	uriParts;

	uriParts = splitPathDir(req->getRequestLine().requestURI);
	for (size_t i = 0; i < uriParts.size(); i++) // lookup rooted Locations & replace if found
	{
		std::map<std::string, std::string>::iterator it;
		it = _rootedLocations.find(uriParts[i]);
		if (it != _rootedLocations.end())
		{
			std::cout << "\t - replace: " << uriParts[i] << " <-> " << it->second << std::endl;
			uriParts[i] = it->second;
		}
	}
	for (size_t i = 0; i < uriParts.size(); i++) // create new uri
	{
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
		|| methodName == "DELETE")
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
AMethod	*MethodExecuter::createMethod(const std::string &methodName, const std::string &path)
{
	std::cout << "MethodExecuter::createMethod() -> " << methodName << std::endl;
	AMethod		*tempMethod = NULL;
	t_Location	*location = NULL;

	location = availableLocation(path);
	if (location != NULL)
	{
		if (isAllowedMethod(location, methodName) && methodName == "GET") // Implement for all Methods
		{
			tempMethod = createGet(methodName, location);
			return tempMethod;
		}
		else
			return NULL;
		std::cout << "location.root = " << location->root << std::endl;
	}
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

/**
	* @brief splits the path into subpaths and searches for a 
	*	fitting t_Location struct.
	* @returns most fitting location struct.
*/
t_Location	*MethodExecuter::availableLocation(const std::string &path)
{
	std::cout << "MethodExecuter::availableLocation(), path = " << path << std::endl;
	t_Location	*loc;
	loc = NULL;

	std::vector<std::string>	pathParts;
	pathParts = splitPath(path);
	for (size_t i = 0; i < _serverConfig->locations.size(); i++)
	{
		for (size_t j = 0; j < pathParts.size(); j++)
		{
			if (_serverConfig->locations[i].path == pathParts[j])
				loc = &_serverConfig->locations[i];
		}
	}
	if (loc != NULL) // print result
		std::cout << "\t ==> location " << loc->root << " {..}" << std::endl;
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
	std::vector<std::string>	parts;
	std::string	temp;
	size_t	start = 0;
	size_t	end = 0;

	for (size_t i = 0; i < path.size(); i++)
	{
		if (path[i] == '/')
		{
			end = i+1;
			temp = path.substr(start, end - start);
			parts.push_back(temp);
			start = end;
		}
	}
	// for (size_t i = 0; i < parts.size(); i++) // print parts
	// 	std::cout << "\tpart[" << i << "] = " << parts[i] << std::endl;
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