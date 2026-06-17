#include "configFileParser.hpp"

ConfigFileParser::ConfigFileParser(){}

ConfigFileParser::~ConfigFileParser(){}

void	ConfigFileParser::parseFile(const std::string &filePath)
{
	// check filename
	std::ifstream	fs(filePath.c_str());
	std::string		str;
	std::string		buffer;

	while (std::getline(fs, buffer))
		str += buffer + '\n';

	std::cout << str << std::endl;
	// tokenize
	tokenize(str);
	adjustTokens();
	printTokens();
	// validate_tokens()
	parseToDataStructure();

}

void ConfigFileParser::tokenize(const std::string &input)
{
	std::cout << "Tokenization" << std::endl;
	size_t start = 0;
	size_t end = 0;
	bool	inToken = false;
	for (size_t i = 0; i < input.size(); i++)
	{
		// std::cout << input[i];
		if (isValidChar(input[i]))
		{
			if (!inToken)
			{
				start = i;
				inToken = true;
			}
		}
		else
		{
			if (inToken)
			{
				end = i;
				t_Token	tok;
				tok.val = input.substr(start, end - start);
				tok.type = getTokenType(tok.val);
				_tokens.push_back(tok);
				inToken = false;
				if (input[i] == ',')
				{
					start = i;
					inToken = true;
				}
			}
			else if ((input[i] != ' ' && input[i] != '\n')&& !inToken)
				std::cout << "SYNTAX_ERROR = " << input[i] << std::endl;
		}
	}
	if (inToken)
	{
		t_Token	tok;
		tok.val = input.substr(start, input.size() - start);
		tok.type = getTokenType(tok.val);
		_tokens.push_back(tok);
	}
}

bool ConfigFileParser::isValidChar(char c)
{
	if (c == ' ' || c == ';' || c == '\n' || c == ',')
		return false;
	if (c >= 'A' && c <= 'z')
		return true;
	if (c == '{' || c == '}')
		return true;
	if (c == '=' || c == '/' || c == ':' || c == '_' || c == '.')
		return true;
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

e_TokenType	ConfigFileParser::getTokenType(std::string tokenStr)
{
	if (tokenStr == "server")
		return SERVER;
	if (tokenStr == "location")
		return LOCATION;
	if (tokenStr == "{")
		return BRACE_OPEN;
	if (tokenStr == "}")
		return BRACE_CLOSE;
	if (tokenStr == "=")
		return ASSIGN;
	if (tokenStr == ",")
		return COMMA;
	return STR;
}

std::string ConfigFileParser::printTokenType(e_TokenType type)
{
	if (type == SERVER)
		return "SERVER";
	if (type == LOCATION)
		return "LOCATION";
	if (type == BRACE_OPEN)
		return "BRACE_OPEN";
	if (type == BRACE_CLOSE)
		return "BRACE_CLOSE";
	if (type == IDENTIFIER)
		return "IDENTIFIER";
	if (type == ASSIGN)
		return "ASSIGN";
	if (type == VALUE)
		return "VALUE";
	if (type == STR)
		return "STR";
	if (type == COMMA)
		return "COMMA";
	return "UNKNOWN";
}

void ConfigFileParser::adjustTokens()
{
	for (size_t i = 0; i < _tokens.size(); i++)
	{
		if (_tokens[i].type == ASSIGN)
		{
			if (_tokens[i - 1].type == STR)
				_tokens[i - 1].type = IDENTIFIER;
			if (_tokens[i + 1].type == STR)
				_tokens[i + 1].type = VALUE;
		}
	}
}

void ConfigFileParser::printTokens()
{
	for (size_t i = 0; i < _tokens.size(); i++)
	{
		std::cout << printTokenType(_tokens[i].type) << " => " << _tokens[i].val << std::endl;
	}
}


// ==================================================
// PARSE ////////////////////////////////////////////
// ==================================================

void ConfigFileParser::parseToDataStructure()
{
	bool inServer = false;

	for (size_t i = 0; i < _tokens.size(); i++)
	{
		if (_tokens[i].type == SERVER && !inServer)
		{
			i = createServer(i);
		}
	}
	printServer();
}

size_t ConfigFileParser::createServer(size_t current_token)
{
	size_t	j;

	for (j = current_token; j < _tokens.size(); j++)
	{
		// if (_tokens[j].type == LOCATION)
		// {
		// 	j += createLocation(j);
		// }
		if (_tokens[j].type == ASSIGN)
		{
			if (checkIdentifier(_tokens[j-1].val))
				setValue(_tokens[j-1].val, j);
		}
		if (_tokens[j].type == LOCATION)
		{
			std::cout << _tokens[j+1].val << std::endl;
			t_Location newLocationObj;
			newLocationObj = createLocation(j);
		}
	}
	return j;
}

t_Location	ConfigFileParser::createLocation(size_t i)
{
	
}

bool	ConfigFileParser::checkIdentifier(const std::string identifier)
{
	if (identifier == "listen"
		|| identifier == "server_name"
		|| identifier == "error_page"
		|| identifier == "client_max_body_size")
		return true;
	return false;
}

void ConfigFileParser::setValue(const std::string id, size_t j)
{
	if (id == "listen")
		_server.listenInterfaces.push_back(_tokens[j + 1].val);
	else if (id == "server_name")
		_server.serverName = _tokens[j+1].val;
	else if (id == "client_max_body_size")
		_server.maxBodySize = convertStrToSize(_tokens[j + 1].val);
	// else if (id == "error_page")
		// _server.errorPages.push_back(_tokens[j
}

size_t	ConfigFileParser::convertStrToSize(const std::string value)
{
	size_t	size;
	std::stringstream ss(value);

	ss >> size;
	return size;
}

void ConfigFileParser::printServer()
{
	std::cout << _server.serverName << std::endl;
	std::cout << _server.maxBodySize << std::endl;
	for (size_t i = 0; i < _server.listenInterfaces.size(); i++)
		std::cout << _server.listenInterfaces[i] << std::endl;
}

