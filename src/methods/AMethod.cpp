#include "AMethod.hpp"

AMethod::AMethod(): _location(NULL)
{
	std::cout << "AMethod::AMethod() -> default constructed" << std::endl;
}

AMethod::AMethod(std::string name): _method(name), _location(NULL), _isAutoIndex(false)
{
	std::cout << "AMethod::AMethod() -> parameterized constructed " << _method << std::endl;
}

AMethod::~AMethod(){}

/**
    * sets the needed data for methods
**/
bool    AMethod::setRequiredData(HttpRequest *req, const std::string modifiedURI)
{
	std::cout << "AMethod::setRequiredData()" << std::endl;

	setHeaders(req->getRequestHeaders());
	setBody(req->getParsedBody());
	setContentData(req->getContentData());
	setResource(modifiedURI);
    setReqUri(req->getRequestLine().requestURI);
	// std::cout << "\tresource = " << _resource << ", modifiedURI = " << modifiedURI << std::endl;

	return true;
}

bool    AMethod::isUploadLocation()
{
    return _location->upload;
}

/**
	* @brief Sets the Identified Resource
*/
void	AMethod::setResource(const std::string &modifiedURI)
{
	// std::cout << "AMethod::setResource()" << std::endl;
	_resource = "." + modifiedURI;
}

void    AMethod::setHeaders(std::map<std::string, std::vector<std::string> > &heads)
{
	_headers = heads;
    // std::cout << "setHeaders..." << std::endl;
}

void    AMethod::setBody(std::map<std::string, s_FormField> &parsedBody)
{
    _parsedBody = parsedBody;
    // std::cout << "setBody..." << std::endl;
}

void    AMethod::setContentData(s_ContentData contentData)
{
    _contentData = contentData;
}

void    AMethod::setReqUri(const std::string &requestURI)
{
    _reqUri = requestURI;
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

std::string AMethod::getRedirectURL()
{
    return _location->redirectURL;
}

std::string AMethod::getLastModified()
{
    return _lastModified;
}

std::string AMethod::getEtag()
{
    return _etag;
}

bool    AMethod::isDirList()
{
    return _isAutoIndex;
}
// bool AMethod::execute()
// {
// 	std::cout << "AMethod::execute() called... " << std::endl;
// }