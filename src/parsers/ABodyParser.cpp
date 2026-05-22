#include "ABodyParser.hpp"

ABodyParser::~ABodyParser()
{}

std::map<std::string, s_FormField> ABodyParser::getResult()
{
	return _result;
}

void	ABodyParser::setContentData(s_ContentData &contentData)
{
	_contentData = contentData;
}
