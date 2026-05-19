#ifndef A_BODY_PARSER_HPP
# define A_BODY_PARSER_HPP
# include <iostream>

class ABodyParser
{
	public:
		virtual ~ABodyParser();
		virtual bool parse(std::string &body) = 0;
};

#endif