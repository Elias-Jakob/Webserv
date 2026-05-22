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
        _code = "403";
        _phrase = "Forbidden";
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

	if (stat(_resource.c_str(), &fileInfo) != 0)
	{
		_code = "404";
		_phrase = "Not Found";
		return false;
    }
	if (S_ISDIR(fileInfo.st_mode))
	{
		_code = "403";
		_phrase = "Forbidden";
		_body = "Cannot delete directories";
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
	if (unlink(_resource.c_str()) != 0) {
		_code = "500";
		_phrase = "Internal Server Error";
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
	_code = "204";
    _phrase = "No Content";
    _body = "";
}