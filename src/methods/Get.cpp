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
		_code = "404";
		_phrase = "NOT FOUND";
		return false;
	}
	// if the request is a directory check for index.html
	if (S_ISDIR(fileInfo.st_mode))
        _resource += "/index.html";
	if (!isFileAccessible(_resource))
	{
		_code = "403";
		_phrase = "Forbidden";
		return false;
	}
	std::ifstream stream(_resource.c_str());
	std::string line;
	if (!stream)
	{
		_code = "404";
		_phrase = "NOT FOUND";
		return false;
	}

	while (std::getline(stream, line))
		_body += line + "\n";

	stream.close();    
	_code = "200";
	_phrase = "OK";
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
