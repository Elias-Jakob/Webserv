#include "Head.hpp"

Head::Head() : Get()
{
	_method = "HEAD";
}

Head::Head(std::string name) : Get()
{
	_method = name;
}

Head::Head(std::string name, t_Location *location) : Get()
{
	_method = name;
	_location = location;
}

Head::~Head()
{}

bool	Head::serveFile(struct stat &fileInfo) 
{
	return setFileHeaders(fileInfo);
}