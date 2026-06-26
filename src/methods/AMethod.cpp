#include "AMethod.hpp"

AMethod::AMethod(): _location(NULL)
{
	std::cout << "AMethod constructed" << std::endl;
}

AMethod::AMethod(std::string name): _method(name), _location(NULL)
{
	std::cout << "AMethod constructed " << _method << std::endl;
}

AMethod::~AMethod(){}

/**
    * sets the needed data for methods
**/
bool    AMethod::setRequiredData(
                        s_RequestLine &reqLine,
                        std::map<std::string,
                        std::vector<std::string> > &reqHeads,
                        std::map<std::string, s_FormField> &parsedResult,
                        s_ContentData &contentData)
{
    std::cout << "setting data for Method..." << std::endl;
    setResource(reqLine.requestURI, reqHeads["Host"][0]);
    setHeaders(reqHeads);
    setBody(parsedResult);
    setContentData(contentData);
    return true;
}

void AMethod::setResource(std::string &reqURI, std::string &host)
{
    if (_location != NULL)
    {
        std::cout << "setting _resource from _location->root" << std::endl;
        _resource = "." + _location->root;
        std::cout << "AMethod::setResource(), _resource = " << _location->root << std::endl;
    }
    else if (_method == "DELETE")
        _resource = "./www" + reqURI;
	else if (host != ".")
		_resource = "." + reqURI;
	// std::cout << "setResource -> " << _resource << std::endl;
}

void AMethod::setHeaders(std::map<std::string, std::vector<std::string> > &heads)
{
	_headers = heads;
    // std::cout << "setHeaders..." << std::endl;
}

void AMethod::setBody(std::map<std::string, s_FormField> &parsedBody)
{
    _parsedBody = parsedBody;
    // std::cout << "setBody..." << std::endl;
}

void AMethod::setContentData(s_ContentData contentData)
{
    _contentData = contentData;
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