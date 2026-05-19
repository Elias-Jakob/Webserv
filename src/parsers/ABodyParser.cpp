#include "ABodyParser.hpp"

ABodyParser::~ABodyParser()
{}

std::map<std::string, s_FormField> ABodyParser::getResult()
{
	return _result;
}