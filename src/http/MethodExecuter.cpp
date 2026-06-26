#include "MethodExecuter.hpp"

MethodExecuter::MethodExecuter()
{}

MethodExecuter::~MethodExecuter()
{}

/**
	* Sets the needed data from request-message, to execute the Method.
	* Calls the method->execute(), and stores the result in local variable.
	* Puts data from method->execute into a t_executionResult result.
	* RETURNS t_executionResult result.
**/
t_executionResult MethodExecuter::execute(AMethod *method, HttpRequest *request)
{
	t_executionResult	result;
	
	std::cout << "MethodExecuter::execute()" << std::endl;

	method->setRequiredData(request->getRequestLine(),
							request->getRequestHeaders(),
							request->getParsedBody(),
							request->getContentData());
	result.success = method->execute();
	// if (result.success)
	// {
		result.statusCode = method->getCode();
		result.statusPhrase = method->getPhrase();
		result.body = method->getBody();
		result.contentType = method->getContentType();
	// }
	return result;
}

/** 	HELPER
	* @brief splits the path at all '/' and returns it for lookup
*/
std::vector<std::string> splitPath(const std::string &path)
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
	for (size_t i = 0; i < parts.size(); i++) // print parts
		std::cout << "\tpart[" << i << "] = " << parts[i] << std::endl;
	return parts;
}

/**
	* @brief splits the path into subpaths and searches for a t_Location.
	* @returns most fitting location struct.
*/
t_Location	*MethodExecuter::availableLocation(const std::string &path)
{
	t_Location	*loc;
	loc = NULL;

	std::vector<std::string>	pathParts;
	pathParts = splitPath(path);
	std::cout << "checking available location (" << path << ")." << std::endl;
	for (size_t i = 0; i < _serverConfig->locations.size(); i++)
	{
		for (size_t j = 0; j < pathParts.size(); j++)
		{
			// std::cout << "comparing (" << _serverConfig->locations[i].path << " <-> " << pathParts[j] << std::endl;
			if (_serverConfig->locations[i].path == pathParts[j])
				loc = &_serverConfig->locations[i];
		}
	}
	if (loc != NULL)
		std::cout << "FOUND LOCATION -> " << loc->root << std::endl;
	else
		std::cout << "NOT FOUND LOCATION" << std::endl;
	return loc;
}

/**
	* Checks if the requested method is valid.
	* RETURNS	- true (if valid method).
				- false (valid is not known or possible).
**/
bool	MethodExecuter::isValidMethod(const std::string &methodName)
{
	if (methodName == "GET"
		|| methodName == "POST"
		|| methodName == "DELETE")
		return true;
	return false;
}

bool	MethodExecuter::isAllowedMethodInLocation(t_Location *location, const std::string &method)
{
	for (size_t i = 0; i < location->allowedMethods.size(); i++)
	{
		if (method == location->allowedMethods[i])
		{
			std::cout << "MethodExecuter()::isAllowedMethodInLocation -> TRUE" << std::endl;
			return true;
		}
	}
	std::cout << "MethodExecuter()::isAllowedMethodInLocation -> FALSE" << std::endl;
	return false;
}

/**
	* Takes the Method-name and creates a new Method object.
	* RETURN	AMethod *method.
**/
AMethod	*MethodExecuter::createMethod(const std::string &methodName, const std::string &path)
{
	AMethod		*tempMethod = NULL;

	// implementation of location object
	t_Location	*location = NULL;
	location = availableLocation(path);
	if (location != NULL)
	{
		if (isAllowedMethodInLocation(location, methodName) && methodName == "GET")
		{
			std::cout << methodName << " is an allowed method in " << path << std::endl;
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

bool	MethodExecuter::setConfig(t_Server *serverConfig)
{
	_serverConfig = serverConfig;
	std::cout << "MethodExecuter::setConfig(): server_name = " << _serverConfig->serverName << std::endl;
	return true;
}

