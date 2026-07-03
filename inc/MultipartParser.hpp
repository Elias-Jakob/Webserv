# ifndef MULTIPART_PARSER_HPP
# define MULTIPART_PARSER_HPP
# include <iostream>
# include <map>
# include <vector>
# include <fstream>
# include <string>
# include <algorithm>
# include "ABodyParser.hpp"

typedef struct	s_extractedData
{
	std::map<
		std::string,
		std::map<
			std::string, 
			std::string> > headers;
	std::string	content;
}				t_extractedData;

/**
	* parses a body of type multipart/form-data
**/
class MultipartParser : public ABodyParser
{
	private:
		// std::map<std::string, std::string> _result;
		bool	findFirstBoundary(std::string &body, size_t *partStart, std::string &fullBoundary);
		bool	extractHeadersAndContent(s_extractedData &data, std::string &body, size_t partStart, std::string &fullBoundary, size_t *nextBoundary);
		bool	createFormField(t_extractedData &data);

	public:
		MultipartParser();
		~MultipartParser();

		bool	parse(std::string &str);
		void	printParsedResult();
		// std::map<std::string, std::string> getResult();
};

#endif