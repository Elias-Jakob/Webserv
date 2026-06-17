#ifndef DELETE_HPP
# define DELETE_HPP

# include "AMethod.hpp"
# include <sys/stat.h> // stat
# include <fcntl.h> // unlink
# include <iostream>
# include <unistd.h>
# include <limits.h>
# include <stdlib.h> // realpath()
# include <errno.h>
# include <stdio.h>

class Delete : public AMethod
{
	private:

	public:
		Delete();
		Delete(std::string name);
		~Delete();

		bool	execute(void);

		bool	resourceExistsAndIsFile();
		bool	isDeletable(const std::string &path);
		bool	deleteResource(void);
		void	setSuccess(void);
};

#endif