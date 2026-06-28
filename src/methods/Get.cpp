#include "Get.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

Get::Get() : AMethod()
{
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
*/
bool Get::execute()
{
	std::cout << "Get::execute() -> resource: " << _resource << std::endl;
	if (_location && _location->redirect) // redirect
	{
		HttpStatus::setStatus(_location->redirectCode, _code, _phrase);
		std::cout << "REDIRECT -> code: " << _code << " ,phrase: " << _phrase << std::endl;
		return false;
	}

	struct stat fileInfo;

	if (stat(_resource.c_str(), &fileInfo) != 0)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	if (S_ISDIR(fileInfo.st_mode)) // resource is directory.
	{
		if (_location->defaultPage.size() > 0)
			_resource += "/" + _location->defaultPage; // check before adding
		else
		{
			std::string tmpResource;
			tmpResource = _resource + "/index.html";
			if (stat(tmpResource.c_str(), &fileInfo) != 0) // create directory listing
			{
				if (_location->autoIndex)
				{
					_body = directoryListing(_resource, _reqUri);
					_isAutoIndex = true;
					HttpStatus::setStatus(200, _code, _phrase);
					return true;
				}
				else
				{
					HttpStatus::setStatus(403, _code, _phrase);
					return false;
				}
			}
			else
				_resource = tmpResource;
		}
	}
	if (!isFileAccessible(_resource))
	{
		std::cout << "\tFile not Accessible" << std::endl;
		HttpStatus::setStatus(403, _code, _phrase);
		return false;
	}
	if (checkCGI()) // CGI extension check here
	{
		return true;
	}
	std::ifstream resourceStream(_resource.c_str(), std::ios::binary);
	if (!resourceStream)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}

	std::string buffer(fileInfo.st_size, '\0');
	resourceStream.read(&buffer[0], fileInfo.st_size);
	_body = buffer;
	
	time_t modTime = fileInfo.st_mtime; // Last-Modified header-field
	_lastModified = convertTimeToHttpDate(modTime);

	std::stringstream ss; // ETag header-field
	ss << fileInfo.st_ino << "-" << fileInfo.st_size << "-" << fileInfo.st_mtime;
	_etag = "\"" + ss.str() + "\"";

	resourceStream.close();
	HttpStatus::setStatus(200, _code, _phrase);
	return true;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

/**
	* @brief Converts identified-Resource & _location->root's relative-PATH 
	*	to the Real-PATH. (./www/file.html => /dirOne/ProjectFolder/www/file.html)
	* @param path -> identified resource (./www/file.html)
	* @return false (if realpath could not be created 
					|| resource-PATH & root-PATH not matching).
*/
bool Get::isFileAccessible(const std::string &path)
{
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
	std::cout << "Get::directoryListing()" << std::endl;
	std::cout << "\tdir: " << dirPath << "; uri: " << uriPath << std::endl;
	DIR				*dir;
	std::string		html;
	struct dirent	*entry;

	dir = opendir(dirPath.c_str());
	if (!dir)
		return "";
	html = "<html><body><h1>Index of " + uriPath + "</h1><ul>";
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == ".")
			continue ;
		html += "<li><a href=\"" + uriPath + "/" + name + "\">" + name + "</a></li>";
	}
	closedir(dir);
	html += "</ul></body></html>";
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

/**
*/
bool	Get::checkCGI()
{
	std::cout << "Get::checkCGI()" << std::endl;
	if (_location->cgiExtensions.size() < 1)
		return false;
	std::string fileExt;
	
	fileExt = _resource.substr(_resource.find_last_of('.'));
	for (size_t i = 0; i < _location->cgiExtensions.size(); i++)
	{
		if (_location->cgiExtensions[i] == fileExt)
		{
			return executeCGI(_resource);
		}
	}
	return false;
}

/**
*/
bool	Get::executeCGI(const std::string &script)
{
	std::cout << "Get::executeCGI()\n\texecution for " << script << " would happen here" << std::endl;
	HttpStatus::setStatus(501, _code, _phrase);
	return true;
}