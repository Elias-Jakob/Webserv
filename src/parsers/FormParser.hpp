#ifndef FORM_PARSER_HPP
# define FORM_PARSER_HPP
# include <iostream>
# include <map>
# include <vector>
# include <fstream>
# include <string>
# include <algorithm>
# include "ABodyParser.hpp"

class FormParser : public ABodyParser
{
	private:
		// std::map<std::string, std::string>	_result;
		void urlDecode(std::string &date);

	public:
		FormParser();
		~FormParser();

		bool parse(std::string &str);
		// std::map<std::string, std::string> getResult();
		void	appendToFile(std::string filename); // should not be in parser but in POST or HTTPREQUEST
};

#endif