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

/**
	* Takes the Method-name and creates a new Method object.
	* RETURN	AMethod *method.
**/
AMethod	*MethodExecuter::createMethod(const std::string &methodName)
{
	AMethod *tempMethod = NULL;

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

AMethod *MethodExecuter::createPost(std::string name)
{
	return new Post(name);
}

AMethod *MethodExecuter::createDelete(std::string name)
{
	return new Delete(name);
}
