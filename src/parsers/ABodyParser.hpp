#ifndef A_BODY_PARSER_HPP
# define A_BODY_PARSER_HPP
# include <iostream>
# include <map>
# include "../../structs.h"
# include "../../print_controls.hpp"

class ABodyParser
{
	protected:
		std::map<std::string, s_FormField>	_result;
		s_ContentData	_contentData;

	public:
		virtual ~ABodyParser();

		virtual bool parse(std::string &body) = 0;

		std::map<std::string, s_FormField> getResult();

		void	setContentData(s_ContentData &contentData);
};


#endif