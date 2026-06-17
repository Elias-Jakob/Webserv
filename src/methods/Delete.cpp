#include "Delete.hpp"

Delete::Delete() : AMethod()
{}

Delete::Delete(std::string name) : AMethod()
{
	_method = name;
	std::cout << "DELETE method constructed" << std::endl;
}

Delete::~Delete(){}

/**
	* Executes the DELETE-Method.
	* checks if the resource is deleteable and deletes is
		if possible.
**/
bool	Delete::execute()
{
	if (!resourceExistsAndIsFile())
		return false;   
    if (!isDeletable(_resource))
	{
		HttpStatus::setStatus(403, _code, _phrase);
        return false;
    }
	if (!deleteResource())
		return false;
	setSuccess();
	return true;
}

/**
	* checks if resource exist and is not a directory
	* RETURN false if not found || resource is directory
**/
bool Delete::resourceExistsAndIsFile(void)
{
	struct stat fileInfo;
	std::cout << "==RESOURCE TO DELETE = " << _resource << std::endl;
	if (stat(_resource.c_str(), &fileInfo) == -1)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	if (S_ISDIR(fileInfo.st_mode))
	{
		HttpStatus::setStatus(403, _code, _phrase);
		_body = "Cannot delete directories";
		return false;
	}
	if (S_ISLNK(fileInfo.st_mode))
	{
		HttpStatus::setStatus(403, _code, _phrase);
    	return false;
	}
	return true;
}

/**
	* Checks if resource is deletable by:
	* 1. checking writeable
	* 2. verify path is within allowed directory
	* 3. if file is in uploads folder
**/
bool Delete::isDeletable(const std::string &path)
{
    char realPath[PATH_MAX];

    if (access(path.c_str(), W_OK) != 0)
        return false;
    if (realpath(path.c_str(), realPath) == NULL)
        return false;    

    std::string resolved(realPath);
    if (resolved.find("/uploads/") == std::string::npos)
        return false;
    return true;
}

/**
	* Deletes resource by using unlink().
**/
bool	Delete::deleteResource()
{
	if (unlink(_resource.c_str()) != 0)
	{
		HttpStatus::setStatus(500, _code, _phrase);
		perror("unlink");
		return false;
	}
	return true;
}

/**
	* Sets the standard code and phrase for a 
		successfull DELETE-request.
**/
void	Delete::setSuccess()
{
	HttpStatus::setStatus(204, _code, _phrase);
    _body = "";
}