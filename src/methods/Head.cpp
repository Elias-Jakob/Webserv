#include "Head.hpp"

Head::Head() : Get()
{
	_method = "HEAD";
	std::cout << "Head -> " << _method << std::endl;
}

Head::Head(std::string name) : Get()
{
	_method = name;
	std::cout << "Head Method constructed" << std::endl;
}

Head::Head(std::string name, t_Location *location) : Get()
{
	_method = name;
	_location = location;
	std::cout << "Head::Head() -> with _location constructed" << std::endl;
}

Head::~Head()
{}

bool	Head::serveFile(struct stat &fileInfo)
{
	std::cout << "Head::serveFile()" << std::endl;
	return setFileHeaders(fileInfo);
}