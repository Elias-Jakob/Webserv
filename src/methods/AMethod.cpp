#include "AMethod.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

AMethod::AMethod(): _method("unset"), _location(NULL), _isAutoIndex(false)
{
    if (METHOD_PRINT)
    	std::cout << "AMethod::AMethod() -> default constructed" << std::endl;
}

AMethod::AMethod(std::string name): _method(name), _location(NULL), _isAutoIndex(false)
{
    if (METHOD_PRINT)
    	std::cout << "AMethod::AMethod() -> parameterized constructed " << _method << std::endl;
}

AMethod::~AMethod()
{}

// =========================================================================
// Public Methods
// =========================================================================

/**
    * @brief Copies all the needed Data frome parsed request to the Method to
    *   be executed.
    * @param req -> Data structure of parsed request.
    * @param modifiedURI -> the already modified request-URI.
*/
bool    AMethod::setRequiredData(HttpRequest *req, const std::string modifiedURI)
{
    if (METHOD_PRINT)
        std::cout << "AMethod::setRequiredData()" << std::endl;
	
    setHeaders(req->getRequestHeaders());
	setBody(req->getParsedBody());
	setContentData(req->getContentData());
	setResource(modifiedURI);
    setReqUri(req->getRequestLine().requestURI);
	return true;
}

/**
    * @brief returns if this->_location can be used for uploads.
*/
bool    AMethod::isUploadLocation()
{
    if (POST_PRINT)
        std::cout << "AMethod::isUploadLocation() => " << _location->upload << std::endl;
    return _location->upload;
}

/**
    * @brief returns if this->_location is for /submit
*/
bool    AMethod::isSubmitLocation()
{
    if (POST_PRINT)
        std::cout << "AMethod::isSubmitLocation() => " << _location->formSubmit << std::endl;
    return _location->formSubmit;
}

/**
    * @brief returns true, if this->_location allowes directory listing.
*/
bool    AMethod::isDirList()
{
    return _isAutoIndex;
}


// =========================================================================
// Getters & Setters
// =========================================================================

/**
    * @brief Checks the file-extensions and returns it.
*/
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

/**
	* @brief Appends '.' before the already modified request-URI.
*/
void	AMethod::setResource(const std::string &modifiedURI)
{
	_resource = "." + modifiedURI;
}

void    AMethod::setHeaders(std::map<std::string, std::vector<std::string> > &heads)
{
	_headers = heads;
}

void    AMethod::setBody(std::map<std::string, s_FormField> &parsedBody)
{
    _parsedBody = parsedBody;
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
