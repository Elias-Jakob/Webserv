#include "Delete.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

Delete::Delete() : AMethod()
{
	_method = "DELETE";
}

Delete::Delete(std::string name) : AMethod()
{
	_method = name;
	std::cout << "DELETE method constructed" << std::endl;
}

Delete::Delete(std::string name, t_Location *location) : AMethod()
{
	_method = name;
	_location = location;
	std::cout << "DELETE method constructed with location" << std::endl;
}

Delete::~Delete(){}

// =========================================================================
// Public Methods
// =========================================================================

/**
  * @brief	Main function of GET-Method, checks if file is deletable
  *			and deletes it.
  * @return	TRUE on success
  *			FALSE on error
*/
bool	Delete::execute()
{
	if (DELETE_PRINT)
	{
		std::cout << "Delete::execute()" << std::endl;
		std::cout << "\tlocation = [" << _location->path << "]" <<std::endl;
	}
	if (!resourceExistsAndIsFile())
		return false;
	if (!isDeletable(_resource))
	{
		HttpStatus::setStatus(403, _code, _phrase);
		return false;
	}
	if (!isUploadLocation())
		return false;
	if (!deleteResource())
		return false;
	setSuccess();
	return true;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

/**
	* checks if resource exist and is not a directory
	* RETURN false if not found || resource is directory
**/
bool Delete::resourceExistsAndIsFile(void)
{
	if (DELETE_PRINT)
		std::cout << "Delete::resourceExistsAndIsFile(), " << _resource << std::endl;
	struct stat fileInfo;
	if (stat(_resource.c_str(), &fileInfo) != 0)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		std::cout << "IS not file or not found" << std::endl;
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
	if (DELETE_PRINT)
		std::cout << "Deleted -> " << _resource << std::endl;
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

	if (access(path.c_str(), W_OK) != 0)
		return false;

    char realPath[PATH_MAX];
	if (realpath(path.c_str(), realPath) == NULL)
		return false;
	
	char	resolvedRoot[PATH_MAX];
	std::string	tmpRoot = "." + _location->root;
	if (realpath(tmpRoot.c_str(), resolvedRoot) == NULL)
		return false;

	std::string docRoot = std::string(resolvedRoot) + '/';
	std::string resolved = std::string(realPath) + '/';
    // std::string resolved(realPath);

	if (resolved.compare(0, docRoot.size(), docRoot))
		return false;
    if (resolved.find("/.") != std::string::npos)
        return false;
    return true;
}

/**
	* Deletes resource by using unlink().
**/
bool	Delete::deleteResource()
{
	std::cout << "would delete Resource" << std::endl;
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

// =========================================================================
// Getters & Setters
// =========================================================================