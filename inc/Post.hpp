#ifndef POST_HPP
# define POST_HPP
# include "AMethod.hpp"
# include <fstream>
# include <sys/stat.h>
# include "FormParser.hpp"
# include "MultipartParser.hpp"

#include <iostream>
// #include <fstream>
#include <vector>
#include <filesystem>
#include <sys/time.h> // gettimeofday
#include <sstream> // int to string
#include <cstdlib> // srand()

class Post : public AMethod
{
	private:
		void	appendToFile(std::string filename); // form POST (./submit)
		void	uploadFile(); // multipart POST (./upload)

	public:
		Post();
		Post(std::string name);
		~Post();

		bool	execute();
		void	printParsedResult();

		std::string	generateRandomFilename(std::string	&recvFilename);
		std::string	extractFileExtension(std::string &recvFilename);
		std::string	getCurrentTime();
		std::string	generateRandomNumber();
		// void	setParsedResult(std::map<std::string, s_FormField> res);
};

/*
	Implement file upload
*/
#endif
