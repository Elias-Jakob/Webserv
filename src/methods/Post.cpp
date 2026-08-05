#include "Post.hpp"

// =========================================================================
// Constructors & Destructor
// =========================================================================

Post::Post() : AMethod()
{
	_method = "POST";
}

Post::Post(std::string name) : AMethod()
{
	_method = name;
	if (POST_PRINT)
		std::cout << "POST" << std::endl;
}

Post::Post(std::string name, t_Location *location)
{
	_method = name;
	_location = location;
	if (POST_PRINT)
		std::cout << "POST with _location constructed" << std::endl;
}

Post::~Post()
{}

// =========================================================================
// Public Methods
// =========================================================================

/**
	* @brief executes Post-Method based on content-type
*/
bool Post::execute()
{
	if (POST_PRINT)
	{
		std::cout << "POST->execute()" << std::endl;
		// printParsedResult();
	}
	if (checkCGI())
		return true;
	if (!isValidContentType())
		return false;
	if (!isUploadLocation() && !isSubmitLocation()) {
		HttpStatus::setStatus(403, _code, _phrase);
		return false;
	}
	if (_contentData.type == "application" && isSubmitLocation()) // submit
	{
		return submitForm();
	}
	else if (_contentData.type == "multipart" || isUploadLocation()) // upload
	{
		return (uploadFile());
	}
	if (POST_PRINT)
		std::cout << _contentData.type << std::endl;
	HttpStatus::setStatus(501, _code, _phrase);
	return false;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

bool	Post::isValidContentType()
{
	if (POST_PRINT)
		std::cout << "Post::isValidContentType() = " << "[" << _contentData.type 
			<< "] [" << _contentData.subtype << "]" << std::endl;
	if (_contentData.type.empty())
	{
		std::cout << "Error: _contentData.type is empty"<< std::endl;
		HttpStatus::setStatus(400, _code, _phrase);
		return false;
	}
	if (_contentData.type == "application")
	{
		if (_contentData.subtype == "x-www-form-urlencoded")
			return true;
		if (_contentData.subtype == "octet-stream") // empty content-type && body
		{ 
			std::cout << "post octet-stream" << std::endl;
			return true;
		}
	}
	if (_contentData.type == "multipart")
	{
		if( _contentData.subtype == "form-data")
			return true;
	}
	std::cout << "NOT VALID" << std::endl;

	HttpStatus::setStatus(415, _code, _phrase);
	return false;
}

bool	Post::submitForm()
{
	if (POST_PRINT)
		std::cout << "Post::submitForm()" << std::endl;
	std::string	formPath;
	formPath = "." + _reqUri;
	if (_location->defaultPage.size() > 0)
		formPath += "/" + _location->defaultPage;
	return appendToFile(formPath);
}

/**
	* @brief handles content-type -> application From a /submit request.
**/
bool Post::appendToFile(std::string filename)
{
	if (POST_PRINT)
		std::cout << "Post::appendToFile(): " << filename << std::endl;
	std::ofstream	output(filename.c_str());
	if(!output.is_open())
	{
		HttpStatus::setStatus(500, _code, _phrase);
		return false;
	}
	std::map<std::string, s_FormField>::iterator it = _parsedBody.begin();
	std::map<std::string, s_FormField>::iterator ite = _parsedBody.end();
	if (it == ite)
		std::cout << "WARNING: it = ite" << std::endl;
	while (it != ite)
	{	std::cout << "write to file" << std::endl;
		output << "hey" << std::endl;
		output << it->first << " = " << it->second.value << std::endl;
		it++;
	}
	output.close();
	HttpStatus::setStatus(201, _code, _phrase);
	return true;
}

/**
	* @brief handles file uploads. generates a filename and creates a file in 
		* upload folder.
**/
bool	Post::uploadFile()
{
	if (POST_PRINT)
		std::cout << "Post::uploadFile()" << std::endl;
	if (_parsedBody.empty()) {
		if(POST_PRINT)
			std::cout << "\t empty body!" << std::endl;
		HttpStatus::setStatus(400, _code, _phrase);
		return false;
	}
	std::map<std::string, s_FormField>::iterator it = _parsedBody.begin();
	std::string	recvFilename = it->second.filename;
	std::cout << "uploadFile:filename = " << recvFilename << std::endl;
	if (_contentData.subtype != "octet-stream" && !isFileNameValid(recvFilename)) // commented out, because of application/octet-stream.
	{
		if (POST_PRINT)
			std::cout << "\t invalid File" << std::endl;
		return false;
	}
	std::string filename = generateRandomFilename(recvFilename);
	if (_location->uploadStore.size() == 0) {
		HttpStatus::setStatus(500, _code, _phrase);
		return false;
	}
	std::string uploadPath = "." + _reqUri + "/";//+ _location->uploadStore + "/";
	std::string	fullPath = uploadPath + filename;
	_uploadLocation = _reqUri + "/" + filename;
	std::cout << "\tuploadPath = " << uploadPath
		<< "\n\tfullPath =" << fullPath << std::endl;
	if (it->second.value.size() > MAX_BODY_SIZE) {
		HttpStatus::setStatus(413, _code, _phrase);
		return false;
	}

	std::ofstream	outFile(fullPath.c_str(), std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		perror("ofstream.open:");
		HttpStatus::setStatus(500, _code, _phrase);
		return false;
	}

	outFile.write(it->second.value.c_str(), it->second.value.size());

	outFile.close();
	HttpStatus::setStatus(201, _code, _phrase);
	return true;
}

/**
	* @brief checks filesize and the extension
// Q: If no uploadExtension set in ConfigFile, allow all or none?
*/
bool	Post::isFileNameValid(const std::string &filename)
{
	if (filename.size() < 1) {
		HttpStatus::setStatus(400, _code, _phrase);
		return false;
	}
	size_t	start;
	start = filename.find_first_of('.', 0);
	if (start == std::string::npos) // no file extension
	{
		if (_location->uploadExtensions.size() == 0) // binary file POST
		{
			std::cout << "WARNING: no fileExtension && no uploadExtensions configured" << std::endl;
			return true;
		}
		HttpStatus::setStatus(400, _code, _phrase);
		return false;
	}
	std::string	extension;
	extension = filename.substr(start, filename.size() - start);
	std::cout << "Post::isFileNameValid()\n\textension: " << extension << std::endl;
	for (size_t i = 0; i < _location->uploadExtensions.size(); i++)
	{
		if (extension == _location->uploadExtensions[i])
			return true;
	}
	HttpStatus::setStatus(415, _code, _phrase);
	return false;
}

/**
	* @brief uses the current time and a random number to create a filename.
**/
std::string	Post::generateRandomFilename(std::string &recvFilename)
{
	std::string	filename;
	std::string	fileExtension;
	std::string	timestamp;
	std::string	randNum;

	fileExtension = extractFileExtension(recvFilename);
	timestamp = getCurrentTime();
	randNum = generateRandomNumber();
	filename = timestamp + "_" + randNum + fileExtension;
	return filename;
}

/**
	* @brief extracts and returns the file-extension (.html, .txt, ..)
**/
std::string	Post::extractFileExtension(std::string &recvFilename)
{
	std::string	extension;
	size_t	start = 0;
	size_t	len = 0;

	start = recvFilename.find_first_of('.', 0);
	if (start == std::string::npos)
		return "";
	if (start < recvFilename.size())
	{
		len = recvFilename.size() - start;
		extension = recvFilename.substr(start, len);
	}
	return extension;
}

/**
	* @brief calls gettimeofday and converts time to ms.
**/
std::string	Post::getCurrentTime()
{
	std::string		timestamp;
	struct timeval	tp;
	long long		usec;
	std::stringstream	ss;

	gettimeofday(&tp, NULL);
	usec = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	if (POST_PRINT)
		std::cout << "usec: " << usec << std::endl;
	ss << usec;
	timestamp = ss.str();
	return timestamp;
}

/**
	* @brief generates random number with rand().
**/
std::string	Post::generateRandomNumber()
{
	unsigned int	randNum;
	std::string		numStr;
	std::stringstream	ss;

	randNum = rand();
	if (POST_PRINT)
		std::cout << "Random number of rand()" << randNum << std::endl;
	ss << randNum;
	numStr = ss.str();
	return numStr;
}

/**
	* prints result of parsing
*/
void	Post::printParsedResult()
{
	std::cout << "\n=== Parsed Multipart Result ===" << std::endl;
	std::cout << "Total fields: " << _parsedBody.size() << std::endl;

	std::map<std::string, t_FormField>::iterator it = _parsedBody.begin();
	std::map<std::string, t_FormField>::iterator ite = _parsedBody.end(); 

	while (it != ite)
	{
		std::cout << "\nField: \"" << it->first << "\"" << std::endl;
		std::cout << "  Type: " << (it->second.isFile ? "FILE" : "TEXT") << std::endl;
		std::cout << "  Content-Type: " << it->second.contentType << std::endl;
		if (it->second.isFile)
		{
			std::cout << "  Filename: " << it->second.filename << std::endl;
			std::cout << "  Size: " << it->second.value.length() << " bytes" << std::endl;
			// std::cout << "  Value: \"" << it->second.value << "\"" << std::endl;
		}
		else
		{
			std::cout << "  Value: \"" << it->second.value << "\"" << std::endl;
		}
		++it;
	}
	std::cout << "==============================\n" << std::endl;
}