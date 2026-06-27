#include "Get.hpp"

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

bool Get::execute()
{
	std::cout << "Get::execute() -> resource: " << _resource << std::endl;
	if (_location && _location->redirect) // redirect
	{
		HttpStatus::setStatus(_location->redirectCode, _code, _phrase);
		std::vector<std::string>	tmp;
		tmp.push_back(_location->redirectURL);
		_headers["Location"] = tmp;
		std::cout << "\t_headers.size = " << _headers.size() << std::endl;
		std::cout << "REDIRECT -> code: " << _code << " ,phrase: " << _phrase << std::endl;
		return false;
	}

	struct stat fileInfo;

	if (stat(_resource.c_str(), &fileInfo) != 0)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	if (S_ISDIR(fileInfo.st_mode))
	{
		if (_location->defaultPage.size() > 0) 
			_resource += "/" + _location->defaultPage; // check before adding
		else
			_resource += "/index.html";
	}
	if (!isFileAccessible(_resource))
	{
		HttpStatus::setStatus(403, _code, _phrase);
		return false;
	}
	std::ifstream stream(_resource.c_str());
	std::string line;
	if (!stream)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}

	while (std::getline(stream, line))
		_body += line + "\n";

	stream.close();    
	HttpStatus::setStatus(200, _code, _phrase);
	return true;
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
	std::cout << "Get::isFileAccessible()\n\tpath = " << path << std::endl;
	if (access(path.c_str(), R_OK) != 0)
		return false;

	char realPath[PATH_MAX];
	if (realpath(path.c_str(), realPath) == NULL)
		return false;

	if (!_location || _location->root.empty())
		return false;

	char resolvedRoot[PATH_MAX];
	std::string	tmpRoot = "." + _location->root;
	if (realpath(tmpRoot.c_str(), resolvedRoot) == NULL)
		return false;

	std::string docRoot = std::string(resolvedRoot) + '/';
	std::string resolved = std::string(realPath) + '/'; // trailing slash so dirs match too
	// std::cout << "docRoot = " << docRoot << "\n"
	// 	<< "resolved = " << resolved << std::endl;

	if (resolved.compare(0, docRoot.size(), docRoot) != 0)
		return false;

	if (resolved.find("/.") != std::string::npos)
		return false;

	return true;
}
