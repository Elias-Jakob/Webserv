#ifndef GET_HPP
# define GET_HPP

# include "AMethod.hpp"
# include <fstream>
# include <sys/stat.h>
# include <unistd.h>
# include <limits.h>
# include <stdlib.h>

class Get : public AMethod
{
	public:
		Get();
		Get(std::string name);
		~Get();

		bool execute();
		bool isFileAccessible(const std::string &path);
};

/*
[] implement safety checks
[] check headers of request

*/
#endif