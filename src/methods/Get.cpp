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

Get::~Get()
{}

bool Get::execute()
{
	struct stat fileInfo;

	if (stat(_resource.c_str(), &fileInfo) != 0)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	// if the request is a directory check for index.html
	if (S_ISDIR(fileInfo.st_mode))
        _resource += "/index.html";
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

bool Get::isFileAccessible(const std::string &path)
{
    // 1. Check if file is readable
    if (access(path.c_str(), R_OK) != 0)
        return false;  // 403 Forbidden
    
    // 2. Resolve real path (prevents ../ tricks)
    char realPath[PATH_MAX];
    if (realpath(path.c_str(), realPath) == NULL)
        return false;

    // 3. Check if inside document root
    // std::string docRoot = "/sgoinfre/cgeringe/webserv/www";  // your root
    // std::string resolved(realPath);
    // if (resolved.find(docRoot) != 0)
    //     return false;  // Path escape attempt!
    
    // 4. Optional: Block hidden files
    // if (resolved.find("/.") != std::string::npos)
    //     return false;
    
    return true;
}
