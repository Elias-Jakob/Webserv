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
**/
bool Post::execute()
{
	if (POST_PRINT)
	{
		std::cout << "POST->execute()" << std::endl;
		printParsedResult();
	}
	if (!isUploadLocation())
		return false;
	if (_contentData.type == "application") // submit
	{
		return (appendToFile("form_input.txt"));
	}
	else if (_contentData.type == "multipart") // upload
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

/**
	* @brief handles content-type -> application From a /submit request.
**/
bool Post::appendToFile(std::string filename)
{
	std::ofstream	output(filename.c_str());
	if(!output.is_open())
	{
		HttpStatus::setStatus(500, _code, _phrase);
		return false;
	}
	std::map<std::string, s_FormField>::iterator it = _parsedBody.begin();
	std::map<std::string, s_FormField>::iterator ite = _parsedBody.end();
	while (it != ite)
	{
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
	if (_parsedBody.empty())
	{
		HttpStatus::setStatus(400, _code, _phrase);
		return false;
	}
	std::map<std::string, s_FormField>::iterator it = _parsedBody.begin();
	std::string	recvFilename = it->second.filename;
	if (!isFileNameValid(recvFilename))
	{
		return false;
	}
	std::string filename = generateRandomFilename(recvFilename);
	if (_location->uploadStore.size() == 0)
	{
		HttpStatus::setStatus(404, _code, _phrase);
		return false;
	}
	std::string uploadPath = "." + _location->uploadStore + "/";
	std::string	fullPath = uploadPath + filename;
	if (POST_PRINT)
	{
		std::cout << "uploadPath: " << uploadPath 
			<< "; fullPath: " << fullPath << std::endl;
	}

	if (it->second.value.size() > MAX_BODY_SIZE)
	{
		HttpStatus::setStatus(413, _code, _phrase);
		return false;
	}

	std::ofstream	outFile(fullPath.c_str(), std::ios::out | std::ios::binary);
	if (!outFile.is_open())
	{
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
	if (filename.size() < 1)
		return false;
	
	size_t	start;
	start = filename.find_first_of('.', 0);
	if (start == std::string::npos) // no file extension
	{
		HttpStatus::setStatus(400, _code, _phrase);
		return false;
	}
	std::string	extension;
	extension = filename.substr(start, filename.size() - start);
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
	// std::cout << "File-Extension: " << fileExtension << std::endl;
	timestamp = getCurrentTime();
	// std::cout << "timestamp: " << timestamp << std::endl;
	randNum = generateRandomNumber();
	// std::cout << "randomNumber: " << randNum << std::endl;
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
	// validation checks?
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
	long int		usec;
	std::stringstream	ss;

	gettimeofday(&tp, NULL);
	usec = tp.tv_sec * 1000 + tp.tv_usec /1000;
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