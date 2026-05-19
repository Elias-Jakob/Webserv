#include "AMethod.hpp"

AMethod::AMethod()
{
	std::cout << "AMethod constructed" << std::endl;
}

AMethod::AMethod(std::string name): _AMethod(name)
{
	std::cout << "AMethod constructed " << _AMethod << std::endl;
}

AMethod::~AMethod()
{}

void AMethod::setResource(std::string &reqURI, std::string &host)
{
	if (host != ".")
		_resource = "." + reqURI;
	std::cout << "setResource -> " << _resource << std::endl;
}

void AMethod::setHeaders(std::map<std::string, std::vector<std::string> > &heads)
{
	_headers = heads;
    std::cout << "setHeaders..." << std::endl;
}

void AMethod::setBody(std::string &body)
{
    _body = body;
    std::cout << "setBody..." << std::endl;
}

std::string &AMethod::getBody()
{
	return _body;
}

std::string &AMethod::getPhrase()
{
	return _phrase;
}

std::string &AMethod::getCode()
{
	return _code;
}

std::string AMethod::getContentType()
{
	size_t pos = _resource.find_last_of('.');
    if (pos == std::string::npos)
        return "application/octet-stream";  // default
    
    std::string ext = _resource.substr(pos);
    
    if (ext == ".html" || ext == ".htm")    return "text/html";
    if (ext == ".css")                      return "text/css";
    if (ext == ".js")                       return "application/javascript";
    if (ext == ".json")                     return "application/json";
    if (ext == ".png")                      return "image/png";
    if (ext == ".jpg" || ext == ".jpeg")    return "image/jpeg";
    if (ext == ".gif")                      return "image/gif";
    if (ext == ".txt")                      return "text/plain";
    if (ext == ".pdf")                      return "application/pdf";
    
    return "application/octet-stream";
}
// bool AMethod::execute()
// {
// 	std::cout << "AMethod::execute() called... " << std::endl;
// }