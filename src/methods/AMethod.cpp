#include "AMethod.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

AMethod::AMethod() : 
    _method("unset"), 
    _location(NULL), 
    _isAutoIndex(false)
{
}

AMethod::AMethod(std::string name) : 
    _method(name), 
    _location(NULL), 
    _isAutoIndex(false)
{
}

AMethod::~AMethod()
{
}

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
    return _location->upload;
}

/**
    * @brief returns if this->_location is for /submit
*/
bool    AMethod::isSubmitLocation()
{
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
    
    if (ext == ".html" || ext == ".htm")
        return "text/html";
    if (ext == ".css")
        return "text/css";
    if (ext == ".js")
        return "application/javascript";
    if (ext == ".json")                     
        return "application/json";
    if (ext == ".png")                      
        return "image/png";
    if (ext == ".jpg" || ext == ".jpeg")    
        return "image/jpeg";
    if (ext == ".gif")                      
        return "image/gif";
    if (ext == ".txt")                      
        return "text/plain";
    if (ext == ".pdf")                      
        return "application/pdf";

    return "application/octet-stream";
}

bool	AMethod::checkCGI()
{
	if (_location->cgiExtensions.size() < 1)
		return false;

    std::string fileExt;
	size_t		pos;
	pos = _resource.find_last_of('.');
	if (pos == std::string::npos) {
		return false;
    }

	fileExt = _resource.substr(pos);
	for (size_t i = 0; i < _location->cgiExtensions.size(); i++) {
		if (_location->cgiExtensions[i] == fileExt) {
			return executeCGI(_resource);
		}
	}
	return false;
}

/**
*/
bool	AMethod::executeCGI(const std::string &script)
{
    struct stat resourceInfo;
    if (stat(script.c_str(), &resourceInfo) != 0) {
        HttpStatus::setStatus(404, _code, _phrase);
        return false;
    }
	HttpStatus::setStatus(601, _code, _phrase);
	_phrase = script;
	return true;
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

std::string AMethod::getUploadLocation()
{
    return _uploadLocation;
}