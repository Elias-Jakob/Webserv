#include "Post.hpp"

Post::Post() : AMethod()
{
}

Post::Post(std::string name) : AMethod()
{
	_method = name;
	std::cout << "POST" << std::endl;
}

Post::~Post()
{}

/**
	* check the content type and parse the body appropialtly
**/
bool Post::execute()
{
	std::cout << "POST->execute()" << std::endl;
	printParsedResult();
	if (_contentData.type == "application")
	{
		appendToFile("form_input.txt");
	}
	else if (_contentData.type == "multipart")
	{
		std::cout << "UPLOAD FILE EXECUTION HERE!" << std::endl;
		uploadFile();
	}
	else
	{
		std::cout << "No Post execution implemented!" << std::endl;
	}
	_code = "200";
	_phrase = "OK";
	std::cout << "POST->execute() end\n" << std::endl;
	return true;
}

/**
	* puts the POST-request of type form (./submit) to a file
**/
void Post::appendToFile(std::string filename)
{
	std::ofstream	output(filename.c_str());
	std::map<std::string, s_FormField>::iterator it = _parsedBody.begin();
	std::map<std::string, s_FormField>::iterator ite = _parsedBody.end();

	while (it != ite)
	{
		output << it->first << " = " << it->second.value << std::endl;
		it++;
	}
	output.close();
}

/**
	* POST type of multipart.
	* Creates a file and writes the content received by request inside newfile.
	* 1. Path safety: To avoid Directory Traversal, generate a filename
	* 2. opens file in binary-mode, so file wont be corrupted
	* 3. 
**/
void	Post::uploadFile()
{
	// 1. safety check for filepath
	// 2. create & open file in binary-mode
	std::map<std::string, s_FormField>::iterator it = _parsedBody.begin();
	// std::map<std::string, s_FormField>::iterator ite = _parsedBody.end(); // for iterating through received files

	std::string	recvFilename = it->second.filename;
	std::string filename = generateRandomFilename(recvFilename);

	std::string	uploadPath = "www/uploads/";
	std::string	fullPath = uploadPath + filename;

	std::ofstream	outFile(fullPath.c_str(), std::ios::out | std::ios::binary);

	if (!outFile.is_open())
	{
		perror("ofstream.open:");
		return ;
	}
	outFile.write(it->second.value.c_str(), it->second.value.size());

	std::cout << "== UPLOADED FILE ==" << std::endl;

	outFile.close();
}

/**
	* generates a random Filename by:
	* 1. extract fileextension(".jpeg", ".txt", ...).
	* 2. getting time now in microseconds.
	* 3. creates a random number.
	RETURN generated string (Timestamp_randomNumber.fileextension)
**/
std::string	Post::generateRandomFilename(std::string &recvFilename)
{
	std::string	filename;
	std::string	fileExtension;
	std::string	timestamp;
	std::string	randNum;

	fileExtension = extractFileExtension(recvFilename);
	std::cout << "File-Extension: " << fileExtension << std::endl;
	timestamp = getCurrentTime();
	std::cout << "timestamp: " << timestamp << std::endl;
	randNum = generateRandomNumber();
	std::cout << "randomNumber: " << randNum << std::endl;
	filename = timestamp + "_" + randNum + fileExtension;
	return filename;
}

/**
	* checks and return the received filename for its extension.
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
	* finds out current time microseconds and converts it to string
	* RETURN string of time
**/
std::string	Post::getCurrentTime()
{
	std::string		timestamp;
	struct timeval	tp;
	long int		usec;
	std::stringstream	ss;

	gettimeofday(&tp, NULL);
	usec = tp.tv_sec * 1000 + tp.tv_usec;
	std::cout << "usec: " << usec << std::endl;
	ss << usec;
	timestamp = ss.str();
	return timestamp;
}

/**
	* Generates a random number and converts it to a string
**/
std::string	Post::generateRandomNumber()
{
	unsigned int	randNum;
	std::string		numStr;
	std::stringstream	ss;

	randNum = rand();
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