#include "Post.hpp"

Post::Post() : AMethod()
{
}

Post::Post(std::string name) : AMethod()
{
	_AMethod = name;
	std::cout << "POST" << std::endl;
}

Post::~Post()
{}

/*
1: check the content type and parse the body appropialtly
*/
ABodyParser *createMultiParser();

bool Post::execute()
{
	std::cout << "POST->execute()" << std::endl;
	// std::cout << "body sent by request: " << _body << std::endl;

	// parse_type = _headers["Content-Type"].at(0);
	parse_type = parseContentType(_headers["Content-Type"]);
	if (parse_type == "application/x-www-form-urlencoded")
	{
		std::cout << "Content-Type is accepted!" << std::endl;
		// parse_form_data(_body);
		FormParser	parser;
		if (parser.parse(_body))
			parser.appendToFile("form_input.txt");
	}
	else if (parse_type == "multipart/form-data")
	{
		std::cout << parse_type << std::endl;
		ABodyParser *parserA;
		parserA = createMultiParser();
		parserA->parse(_body);
	}
	else
		std::cout << "PARSE-Type " << parse_type << " is not accepted!" << std::endl;
		// std::cout << "Content-Type is not accepted!" << std::endl;

	_code = "200";
	_phrase = "OK";
	std::cout << "POST->execute() end\n" << std::endl;
	return true;
}

std::string	Post::parseContentType(std::vector<std::string> value)
{
std::cout << "parsing contentType..." << std::endl;
	std::string	temp;
	std::string	parameter;
	std::string type;
	size_t posSemiColon = 0;
	size_t posSlash = 0;

	temp = value.at(0);
	// posSemiColon = temp.find(';', 0);
	// if (pos < temp.size())
	if ((posSlash = temp.find('/', 0)) < temp.size())
	{
		std::cout << "\tfound type/subtype..." << std::endl;
		_contentData.type = temp.substr(0, posSlash);
		if ((posSemiColon = temp.find(';', posSlash)) < temp.size())
		{
			std::cout << "\tfound parameter..." << std::endl;
			parameter = temp.substr(posSemiColon, temp.size() - posSemiColon);
			_contentData.subtype = temp.substr(posSlash + 1, posSemiColon - posSlash);
			size_t posEqual = 0;
			if ((posEqual = parameter.find("=", 0)) < parameter.size())
			{
				std::cout << "\tfound parameter-value..." << std::endl;
				_contentData.boundary = parameter.substr(posEqual + 1, parameter.size() - posEqual);
			}
		}
		else
		{
			std::cout << "\tno parameter found..." << std::endl;
			_contentData.subtype = temp.substr(posSlash + 1, temp.size() - posSlash);
		}
		type = temp.substr(0, posSemiColon);
		parameter = temp.substr(posSemiColon + 1, temp.size());
		std::cout << "s_ContentData = {" << std::endl;
		std::cout << "\ttype: \"" << _contentData.type << "\"" << std::endl;
		std::cout << "\tsubtype: \"" << _contentData.subtype << "\"" << std::endl;
		std::cout << "\tboundary: \"" << _contentData.boundary << "\"\n}" << std::endl;
		std::cout << type << std::endl;
		std::cout << parameter << std::endl;
	}
	return type;
}

ABodyParser *createMultiParser()
{
	return new MultipartParser();
}
