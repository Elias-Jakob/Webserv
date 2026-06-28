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
		bool isFileAccessible(const std::string &path);
		std::string directoryListing(const std::string &dirPath, const std::string &uriPath);
};

/*
[] implement safety checks
[] check headers of request

*/
#endif