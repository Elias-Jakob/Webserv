#ifndef GET_HPP
# define GET_HPP

# include "AMethod.hpp"
# include <fstream>
# include <sys/stat.h>
# include <unistd.h>
# include <limits.h>
# include <stdlib.h>
# include <dirent.h>
# include "../config/ConfigFileParser.hpp"

class Get : public AMethod
{
	public:
		Get();
		Get(std::string name);
		Get(std::string name, t_Location *location);
		~Get();

		bool execute();
	
	private:
		bool		handleRedirect();
		bool		handleDirectory(struct stat &fileInfo);
		bool		serveFile(struct stat &fileInfo);
		bool 		isFileAccessible(const std::string &path);
		bool		checkCGI();
		bool		executeCGI(const std::string &script);
		std::string	directoryListing(const std::string &dirPath, const std::string &uriPath);
		std::string	convertTimeToHttpDate(time_t time);
};

/*
[] implement safety checks
[] check headers of request

*/
#endif