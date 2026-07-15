#include "Head.hpp"

Head::Head() : Get()
{
	_method = "GET";
	std::cout << "GET -> " << _method << std::endl;
}

Head::Head(std::string name) : Get()
{
	_method = name;
	std::cout << "GET Method constructed" << std::endl;
}

Head::Head(std::string name, t_Location *location) : Get()
{
	_method = name;
	_location = location;
	std::cout << "Get::Get() -> with _location constructed" << std::endl;
}

Head::~Head()
{}

bool	Head::serveFile(struct stat &fileInfo)
{
	return setFileHeaders(fileInfo);
}