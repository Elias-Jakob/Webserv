#ifndef GET_HPP
# define GET_HPP

# include "AMethod.hpp"
# include <fstream>
# include <sys/stat.h>
# include <unistd.h>
# include <limits.h>
# include <stdlib.h>
# include <dirent.h>
# include "ConfigFileParser.hpp"

/**
	* @class Get
	* @brief Inherites from AMethod. Reads the Requested file and puts its
	*	content into body for the response.
*/
class Get : public AMethod
{
	public:
		Get();
		Get(std::string name);
		Get(std::string name, t_Location *location);
		~Get();

		bool execute();
		bool		setFileHeaders(struct stat &fileInfo);
	
	private:
		bool		handleRedirect();

		bool		handleDirectory(struct stat &fileInfo);
		bool		serveDefaultPage();
		bool		serveIndexPage();
		bool		serveDirectoryList();
		std::string	directoryListing(const std::string &dirPath, const std::string &uriPath);

		virtual bool		serveFile(struct stat &fileInfo);
		
		bool 		isFileAccessible(const std::string &path);
		bool		checkCGI();
		bool		executeCGI(const std::string &script);
		
		std::string	convertTimeToHttpDate(time_t time);
};

#endif
