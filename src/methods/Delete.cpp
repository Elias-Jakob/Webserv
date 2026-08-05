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
}

Delete::Delete(std::string name, t_Location *location) : AMethod()
{
	_method = name;
	_location = location;
}

Delete::~Delete(){}

// =========================================================================
// Public Methods
// =========================================================================

/**
	* @brief	Main function of DELETE Method. Does checks to verify the file is 
	*		deletable. Deltes requested file and sets _code(status-code) to the
	*		appropiate value.
	* @return	TRUE on success
	*			FALSE on error
*/
bool	Delete::execute()
{
	if (!resourceExistsAndIsFile())
		return false;
	if (!isDeletable(_resource)) {
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
	* @brief Gets file-info with stat() and checks the infos to verify it is a
	*	file. Sets _code to appropiate value if fail.
	* @return FALSE	-> a) file does not exist, is directory, is link
*/
bool Delete::resourceExistsAndIsFile(void)
{
	struct stat fileInfo;
	if (stat(_resource.c_str(), &fileInfo) != 0) {
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	if (S_ISDIR(fileInfo.st_mode)) {
		HttpStatus::setStatus(403, _code, _phrase);
		return false;
	}
	if (S_ISLNK(fileInfo.st_mode)) {
		HttpStatus::setStatus(403, _code, _phrase);
    	return false;
	}
	return true;
}

/**
	* @brief Lexically normalizes _resource and the location's root-directory
	*	(no filesystem access) and compares them to block path traversal.
	* @param path the location of resource to be deleted.
	* @return false: path's dont match up.
				true: success.
*/
bool Delete::isDeletable(const std::string &path)
{

	if (access(path.c_str(), W_OK) != 0)
		return false;

	std::string	docRoot = utils::normalizePath("." + _location->alias);
	std::string	resolved = utils::normalizePath(path);

	if (!docRoot.empty()
		&& resolved != docRoot
		&& resolved.compare(0, docRoot.size() + 1, docRoot + "/") != 0)
		return false;
    return true;
}

/**
	* @brief Deltes requested _resource with unlink().
	* @return true, success.
	*		false, if unlink() failed.
*/
bool	Delete::deleteResource()
{
	if (unlink(_resource.c_str()) != 0) {
		HttpStatus::setStatus(500, _code, _phrase);
		perror("unlink");
		return false;
	}
	return true;
}

/**
	* @brief sets status to 204 No Content & _body for response to empty.
*/
void	Delete::setSuccess()
{
	HttpStatus::setStatus(204, _code, _phrase);
    _body = "";
}
