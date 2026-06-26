#ifndef GET_HPP
# define GET_HPP

# include "AMethod.hpp"
# include <fstream>
# include <sys/stat.h>
# include <unistd.h>
# include <limits.h>
# include <stdlib.h>
# include "../config/ConfigFileParser.hpp"

class Get : public AMethod
{
	public:
		Get();
		Get(std::string name);
		Get(std::string name, t_Location *location);
		~Get();

		bool execute();
		bool isFileAccessible(const std::string &path);
};

/*
[] implement safety checks
[] check headers of request

*/
#endif