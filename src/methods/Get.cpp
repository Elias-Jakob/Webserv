#include "Get.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

Get::Get() : AMethod()
{
	_method = "GET";
	std::cout << "GET -> " << _method << std::endl;
}

Get::Get(std::string name) : AMethod()
{
	_method = name;
	std::cout << "GET Method constructed" << std::endl;
}

Get::Get(std::string name, t_Location *location) : AMethod()
{
	_method = name;
	_location = location;
	std::cout << "Get::Get() -> with _location constructed" << std::endl;
}

Get::~Get()
{}


// =========================================================================
// Public Methods
// =========================================================================

/**
	* @brief Executes GET request: resolves resource, handles redirects/directories/files.
	* @return true if completed successfully, false otherwise (status code set via HttpStatus).
*/
bool	Get::execute()
{
	if (GET_PRINT)
		std::cout << "Get::execute()" << std::endl;
	if (handleRedirect())
		return false;
	
	if (checkCGI())
		return true;
	struct stat fileInfo;
	if (stat(_resource.c_str(), &fileInfo) != 0)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	if (S_ISDIR(fileInfo.st_mode))
	{
		// return handleDirectory(fileInfo);
		if (handleDirectory(fileInfo))
			return true;
		else
			std::cout << "status_code: " << _code << std::endl;
		// else if (_resource != "/")
		// {
		// 	HttpStatus::setStatus(404, _code, _phrase);
		// 	return false;
		// }
	}
	std::cout << "\t_resource" << _resource << std::endl;
	// if (stat(_resource.c_str(), &fileInfo) != 0 && S_ISDIR(fileInfo.st_mode))
	// {
	// 	HttpStatus::setStatus(404, _code, _phrase);
	// 	return false;
	// }
	if (!isFileAccessible(_resource))
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	return serveFile(fileInfo);
}

// =========================================================================
// Private Helper Methods
// =========================================================================

/**
 	* @brief Checks if request is a redirect and sets appropriate response.
 	* @return true if redirect was handled, false otherwise.
*/
bool	Get::handleRedirect()
{
	if (GET_PRINT)
		std::cout << "Get::handleRedirect()" << std::endl;
	if (_location && _location->redirect)
	{
		HttpStatus::setStatus(_location->redirectCode, _code, _phrase);
		std::cout << "REDIRECT -> code: " << _code << " ,phrase: " << _phrase << std::endl;
		return true;
	}
	return false;
}

/**
	* @brief Handles directory requests: checks for: 
	*	1. default page, 
		2. index.html, 
		3. generates directory-listing.
	* @param fileInfo stat structure of the directory
	* @return true if successfully handled, false otherwise.
*/
bool	Get::handleDirectory(struct stat &fileInfo)
{
	if (GET_PRINT)
		std::cout << "Get::handleDirectory()" << std::endl;
	if (_location->defaultPage.size() > 0) // index = yyy.html
	{
		if (GET_PRINT)
			std::cout << "\tServe default-page " << _location->defaultPage << std::endl;
		return serveDefaultPage();
	}

	std::string tmpResource = _resource + "/index.html"; // check if dir has index.html
	if (stat(tmpResource.c_str(), &fileInfo) == 0)
	{
		if (GET_PRINT)
			std::cout << "\tserve dir/index.html" << std::endl;
		_resource = tmpResource;
		return serveIndexPage();
	}

	if (_location->autoIndex) // directory listing
	{
		if (GET_PRINT)
			std::cout << "\tcreate directory-list" << std::endl;
		return serveDirectoryList();
	}

	HttpStatus::setStatus(404, _code, _phrase);
	std::cout << "handleDirectory() => FALSE" << std::endl;
	return false;
}

/**
	* @brief
*/
bool	Get::serveDefaultPage()
{
	if (GET_PRINT)
		std::cout << "Get::serveDefaultPage()\n\t resource: " << _resource << std::endl;
	if (_resource.at(_resource.size()-1) != '/')
		_resource += "/";
	_resource = _resource + _location->defaultPage;
	// _resource = "." + _location->root + "/" + _location->defaultPage;

	struct stat resourceInfo;
	if (stat(_resource.c_str(), &resourceInfo) != 0)
	{
		std::cout << "here" << std::endl;
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	return serveFile(resourceInfo);
}

/** 
*/
bool	Get::serveIndexPage()
{
	std::cout << "Get::serveIndexPage()" << std::endl;
	struct stat resourceInfo;
	if (stat(_resource.c_str(), &resourceInfo) != 0)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	return serveFile(resourceInfo);
}

/**
*/
bool	Get::serveDirectoryList()
{
	_body = directoryListing(_resource, _reqUri);
	_isAutoIndex = true;
	HttpStatus::setStatus(200, _code, _phrase);
	return true;
}

// Extract file reading into a separate helper
bool Get::setFileHeaders(struct stat &fileInfo)
{
    // Set headers without reading content
    _lastModified = convertTimeToHttpDate(fileInfo.st_mtime);
    std::stringstream ss;
    ss << fileInfo.st_ino << "-" << fileInfo.st_size << "-" << fileInfo.st_mtime;
    _etag = "\"" + ss.str() + "\"";
    HttpStatus::setStatus(200, _code, _phrase);
    return true;
}

/**
	* @brief Reads file from disk and sets Last-Modified and ETag headers.
	* @param fileInfo stat structure of the file
	* @return true if successful, false otherwise.
*/
bool	Get::serveFile(struct stat &fileInfo)
{
	if (GET_PRINT)
		std::cout << "Get::serveFile()\n\t _resource = " << _resource << std::endl;
	std::ifstream resourceStream(_resource.c_str(), std::ios::binary);
	if (!resourceStream)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}

	std::string buffer(fileInfo.st_size, '\0');
	resourceStream.read(&buffer[0], fileInfo.st_size);
	_body = buffer;
	resourceStream.close();

	// _lastModified = convertTimeToHttpDate(fileInfo.st_mtime);

	// std::stringstream ss;
	// ss << fileInfo.st_ino << "-" << fileInfo.st_size << "-" << fileInfo.st_mtime;
	// _etag = "\"" + ss.str() + "\"";

	// HttpStatus::setStatus(200, _code, _phrase);
	return setFileHeaders(fileInfo);
}

/**
	* @brief Converts identified-Resource & _location->root's relative-PATH 
	*	to the Real-PATH. (./www/file.html => /dirOne/ProjectFolder/www/file.html)
	* @param path -> identified resource (./www/file.html)
	* @return false (if realpath could not be created 
					|| resource-PATH & root-PATH not matching).
*/
bool Get::isFileAccessible(const std::string &path)
{
	if (GET_PRINT)
		std::cout << "Get::isFileAccessible()\n\tpath = " << path << std::endl;
	if (access(path.c_str(), R_OK) != 0)
		return false;

	char realPath[PATH_MAX];
	if (realpath(path.c_str(), realPath) == NULL)
		return false;

	char resolvedRoot[PATH_MAX];
	std::string	tmpRoot = "." + _location->root;
	if (realpath(tmpRoot.c_str(), resolvedRoot) == NULL)
		return false;

	std::string docRoot = std::string(resolvedRoot) + '/';
	std::string resolved = std::string(realPath) + '/'; // trailing slash so dirs match too

	if (resolved.compare(0, docRoot.size(), docRoot) != 0)
		return false;

	if (resolved.find("/.") != std::string::npos)
		return false;

	return true;
}

/**
	* @brief builds a list of the files of the dirPath and stores in string in an html
	*	format
*/
std::string	Get::directoryListing(const std::string &dirPath, const std::string &uriPath)
{
	if (GET_PRINT)
	{	
		std::cout << "Get::directoryListing()" << std::endl;
		std::cout << "\tdir: " << dirPath << "; uri: " << uriPath << std::endl;
	}
	DIR				*dir;
	std::string		html;
	struct dirent	*entry;

	dir = opendir(dirPath.c_str());
	if (!dir)
		return "";
	html = "<html><body><h1>Index of " + uriPath + "</h1><ul>";
	while ((entry = readdir(dir)) != NULL) // Q: Directory or file entry
	{
		std::string name = entry->d_name;
		if (name == ".")
			continue ;
		html += "<li><a href=\"" + uriPath + "/" + name + "\">" + name + "</a></li>";
	}
	closedir(dir);
	html += "</ul></body></html>";
	std::cout << "html: " << html << std::endl;
	return html;
}

/**
	* @brief Converts time_t to a string in correct format for
	*	the Last-Modified header-field (Response).
*/
std::string	Get::convertTimeToHttpDate(time_t time)
{
	struct tm	*tm_info;
	char buf[100];

	tm_info = gmtime(&time);
	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
	return std::string(buf);
}

// /**
// */
// bool	Get::checkCGI()
// {
// 	std::cout << "Get::checkCGI()" << std::endl;
// 	if (_location->cgiExtensions.size() < 1)
// 		return false;
// 	std::string fileExt;
// 	size_t		pos;

// 	pos = _resource.find_last_of('.');
// 	if (pos == std::string::npos)
// 		return false;
// 	fileExt = _resource.substr(pos);
// 	for (size_t i = 0; i < _location->cgiExtensions.size(); i++)
// 	{
// 		if (_location->cgiExtensions[i] == fileExt)
// 		{
// 			return executeCGI(_resource);
// 		}
// 	}
// 	return false;
// }

// /**
// */
// bool	Get::executeCGI(const std::string &script)
// {
// 	std::cout << "Get::executeCGI()\n\texecution for " << script << " would happen here" << std::endl;
// 	HttpStatus::setStatus(601, _code, _phrase);
// 	_phrase = _resource;
// 	return true;
// }
