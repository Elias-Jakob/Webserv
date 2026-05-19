# ifndef MULTIPART_PARSER_HPP
# define MULTIPART_PARSER_HPP
# include <iostream>
# include <map>
# include <vector>
# include <fstream>
# include <string>
# include <algorithm>
# include "ABodyParser.hpp"

// parses a body of type multipart/form-data
//
class MultipartParser : public ABodyParser
{
	private:
		// std::map<std::string, std::string> _result;

	public:
		MultipartParser();
		~MultipartParser();

		bool parse(std::string &str);
		// std::map<std::string, std::string> getResult();
};

#endif