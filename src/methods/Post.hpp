#ifndef POST_HPP
# define POST_HPP
# include "AMethod.hpp"
# include <fstream>
# include <sys/stat.h>

# include <iostream>
# include <vector>
# include <filesystem>
# include <sys/time.h> // gettimeofday
# include <sstream> // int to string
# include <cstdlib> // srand()

# include "../parsers/FormParser.hpp"
# include "../parsers/MultipartParser.hpp"

class Post : public AMethod
{
	public:
		Post();
		Post(std::string name);
		Post(std::string name, t_Location *location);
		~Post();

		bool	execute();

	private:
		bool	appendToFile(std::string filename); // form POST (./submit)
		bool	uploadFile(); // multipart POST (./upload)
		void	printParsedResult();

		std::string	generateRandomFilename(std::string	&recvFilename);
		std::string	extractFileExtension(std::string &recvFilename);
		std::string	getCurrentTime();
		std::string	generateRandomNumber();
		bool		isFileNameValid(const std::string &filename);

};

/*
	Implement file upload
*/
#endif