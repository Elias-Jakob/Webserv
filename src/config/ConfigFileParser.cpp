#include "ConfigFileParser.hpp"

ConfigFileParser::ConfigFileParser(){}

ConfigFileParser::~ConfigFileParser(){}

t_Configs	&ConfigFileParser::parseFile(const std::string &filePath)
{
	// check filename
	std::ifstream	fs(filePath.c_str());
	std::string		str;
	std::string		buffer;
	if (!fs.is_open())
		throw std::runtime_error(std::strerror(errno));
	while (std::getline(fs, buffer))
		str += buffer + '\n';

	// std::cout << str << std::endl;
	tokenize(str);
	adjustTokens();
	// printTokens();
	// validate_tokens();
	parseToDataStructure();
	parseEndpoints();
	if (PRINT_SERVER_CONFIG)
		printServer();
	return (this->_configs);
}

void ConfigFileParser::tokenize(const std::string &input)
{
	std::cout << "TOKENIZATION" << std::endl;
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
	if (isalpha(c))
		return true;
	if (c == '{' || c == '}')
		return true;
	if (c == '=' || c == '/' || c == ':' || c == '_' || c == '.' || c == '-')
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



void ConfigFileParser::adjustTokens()
{
	for (size_t i = 0; i < _tokens.size(); i++)
	{
		if (_tokens[i].type == ASSIGN && i > 0 && i + 1 < _tokens.size())
		{
			if (_tokens[i - 1].type == STR && isNbr(_tokens[i -1].val))
				_tokens[i - 1].type = NUMBER;
			else if (_tokens[i - 1].type == STR)
				_tokens[i - 1].type = IDENTIFIER;
			if (_tokens[i + 1].type == STR)
				_tokens[i + 1].type = VALUE;
		}
		if (_tokens[i].type == LOCATION && i + 1 < _tokens.size())
			_tokens[i + 1].type = PATH_LOCATION;
	}
}

bool ConfigFileParser::isNbr(const std::string &s)
{
	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] < '0' || s[i] > '9')
			return false;
	}
	return true;
}


// ==================================================
// PARSE ////////////////////////////////////////////
// ==================================================

void ConfigFileParser::parseToDataStructure()
{
	for (size_t i = 0; i < _tokens.size(); i++)
	{
		if (_tokens[i].type == SERVER)
		{
			createServer(&i);
		}
	}
}

size_t ConfigFileParser::createServer(size_t *i)
{
	size_t	j;

	for (j = *i; j < _tokens.size(); j++)
	{
		if (j >= 2 
			&&_tokens[j].type == ASSIGN
			&& _tokens[j - 1].type == NUMBER
			&& _tokens[j - 2].val == "error_page")
		{
			std::stringstream	ss(_tokens[j - 1].val);
			int	errorCode;
			ss >> errorCode;
			_configs.errorPages[errorCode] = _tokens[j + 1].val;
			j += 1;
		}
		else if (_tokens[j].type == ASSIGN)
		{
			if (checkIdentifier(_tokens[j-1].val))
				setValue(_tokens[j-1].val, j);
		}
		else if (_tokens[j].type == LOCATION)
		{
			// std::cout << _tokens[j+1].val << std::endl;
			j += createLocation(j);
		}
	}
	*i += j;
	return j;
}


size_t ConfigFileParser::setLocationVal(size_t i, t_Location *loc)
{
	if (i + 1 >= _tokens.size())
		return i;
	if (_tokens[i - 1].val == "root")
		loc->root = _tokens[i + 1].val;
	if (_tokens[i - 1].val == "accepted_methods")
	{
		size_t	j = i + 1;
		while (j < _tokens.size() && (_tokens[j].type == VALUE || _tokens[j].type == STR))
		{
			loc->allowedMethods.push_back(_tokens[j].val);
			if (j + 1 < _tokens.size() && _tokens[j+1].type == COMMA)
				j += 2;
			else
				break;
		}
		return j;
	}
	if (_tokens[i - 1].val == "cgi_extension")
	{
		size_t	j = i + 1;
		while (j < _tokens.size() && (_tokens[j].type == VALUE || _tokens[j].type == STR))
		{
			loc->cgiExtensions.push_back(_tokens[j].val);
			if (j + 1 < _tokens.size() && _tokens[j+1].type == COMMA)
				j += 2;
			else
				break;
		}
		return j;
	}
	if (_tokens[i - 1].val == "upload_enable")
		loc->upload = true;
	if (_tokens[i - 1].val == "upload_store")
		loc->uploadStore = _tokens[i + 1].val;
	if (_tokens[i -1].val == "upload_extensions")
	{
		size_t	j = i + 1;
		while (j < _tokens.size() && (_tokens[j].type == VALUE || _tokens[j].type == STR))
		{
			loc->uploadExtensions.push_back(_tokens[j].val);
			if (j + 1 < _tokens.size() && _tokens[j+1].type == COMMA)
				j += 2;
			else
				break;
		}
		return j;
	}
	if (_tokens[i -1].val == "index")
		loc->defaultPage = _tokens[i+1].val;
	if (_tokens[i -1].val == "return")
	{
		loc->redirect = true;
		loc->redirectCode = std::atoi(_tokens[i+1].val.c_str());
		if (i+2 < _tokens.size())
			loc->redirectURL = _tokens[i + 2].val;
		return i + 2;
	}
	if (_tokens[i - 1].val == "autoindex" && _tokens[i + 1].val == "on")
		loc->autoIndex = true;
	if (_tokens[i - 1].val == "form_output_file")
	{
		loc->formSubmit = true;
		loc->formUploadFile = _tokens[i + 1].val;
	}
	return i + 1;
}

size_t ConfigFileParser::createLocation(size_t i)
{
	t_Location	tempLoc;
	size_t		j = i;
	
	tempLoc.path = _tokens[i+1].val;
	tempLoc.redirect = false;
	tempLoc.upload = false;
	tempLoc.autoIndex = false;
	tempLoc.formSubmit = false;
	while (_tokens.size() > j && _tokens[j].type != BRACE_CLOSE && _tokens[j].type != END_OF_FILE)
	{
		if (_tokens[j].type == ASSIGN)
		{
			j = setLocationVal(j, &tempLoc);
			j--;
		}
		j++;
	}
	_configs.locations.push_back(tempLoc);
	return (j - i);
}

// accepting multiple Ports on one interface map<string, vector<string>>
void	ConfigFileParser::parseEndpoints()
{
	size_t	seperator = 0;
	for (size_t i = 0; i < _configs.listenInterfaces.size(); i++)
	{
		seperator = _configs.listenInterfaces[i].find(':', 0);
		if (seperator != std::string::npos)
		{
			std::string	s = _configs.listenInterfaces[i];
			std::string	interface;
			std::string	port;

			interface = s.substr(0, seperator);
			port = s.substr(seperator + 1, s.size() - seperator -1);

			std::map<std::string, std::vector<std::string> >::iterator it = _configs.endpoints.find(interface);
			if (it != _configs.endpoints.end())
			{
				it->second.push_back(port);
			}
			else
				_configs.endpoints[interface].push_back(port);//  = port;
		}
	}
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
	if (j + 1 >= _tokens.size())
		return ;
	if (id == "listen")
		_configs.listenInterfaces.push_back(_tokens[j + 1].val);
	else if (id == "server_name")
		_configs.serverName = _tokens[j+1].val;
	else if (id == "client_max_body_size")
		_configs.maxBodySize = convertStrToSize(_tokens[j + 1].val);
}

size_t	ConfigFileParser::convertStrToSize(const std::string value)
{
	size_t	size;
	char	suffix = 0;
	std::stringstream ss(value);
	ss >> size >> suffix;

	if (suffix == 'M' || suffix == 'm')
		size *= 1024 * 1024;
	else if (suffix == 'K' || suffix == 'k')
		size *= 1024;
	return size;
}

void ConfigFileParser::printServer()
{
	std::cout << "\n\nSERVER DATA{" << std::endl;
	std::cout << "name: "<< _configs.serverName << std::endl;
	std::cout << "max_body_size: " << _configs.maxBodySize << std::endl;
	std::map<std::string, std::vector<std::string> >::iterator itIP = _configs.endpoints.begin();
	std::map<std::string, std::vector<std::string> >::iterator itIPe = _configs.endpoints.end();
	std::cout << "Endpoints {\n";
	while (itIP != itIPe)
	{
		std::cout << "\t[" << itIP->first << "] = ";
		for (size_t i = 0; i < itIP->second.size(); i++)
		{
			std::cout << itIP->second[i] << ", ";
		}
		std::cout << std::endl;
		itIP++;
	}
	std::cout << "}" << std::endl;

	std::map<int, std::string>::iterator it = _configs.errorPages.begin();
	std::map<int, std::string>::iterator ite = _configs.errorPages.end();
	std::cout << "errorPages{\n";
	while (it != ite)
	{
		std::cout << "\t[" << it->first << "] = " << it->second << std::endl;
		it++;
	}
	std::cout << "}" << std::endl;
	for (size_t i = 0; i < _configs.locations.size(); i++)
	{
		std::cout << "location {" << std::endl;
		std::cout << "\tpath= " << _configs.locations[i].path << std::endl;
		std::cout << "\troot= " << _configs.locations[i].root << std::endl;
		if (_configs.locations[i].defaultPage.size() > 0)
			std::cout << "\tindex= " << _configs.locations[i].defaultPage << std::endl;
		if (_configs.locations[i].allowedMethods.size() > 0)
		{
			std::cout << "\tmethods= ";
			for(size_t j = 0; j < _configs.locations[i].allowedMethods.size(); j++)
			{
				if (j + 1 == _configs.locations[i].allowedMethods.size())
					std::cout << _configs.locations[i].allowedMethods[j] << std::endl;
				else
					std::cout << _configs.locations[i].allowedMethods[j] << ", ";
			}
		}
		if (_configs.locations[i].redirect)
		{
			std::cout << "\tredirectCode: " << _configs.locations[i].redirectCode << std::endl;
			std::cout << "\tredirectURL: " << _configs.locations[i].redirectURL << std::endl;
		}
		if (_configs.locations[i].upload)
			std::cout << "\tuploadStorage: " << _configs.locations[i].uploadStore << std::endl;
		if (_configs.locations[i].upload)
		{
			std::cout << "\textensions = ";
			for (size_t j = 0; j < _configs.locations[i].uploadExtensions.size(); j++)
			{
				if (j + 1 == _configs.locations[i].uploadExtensions.size())
					std::cout << _configs.locations[i].uploadExtensions[j] << std::endl;
				else
					std::cout << _configs.locations[i].uploadExtensions[j] << ", ";
			}
		}
		if (_configs.locations[i].cgiExtensions.size() > 0)
		{
			std::cout << "\tcgiExtensions: ";
			for (size_t j = 0; j < _configs.locations[i].cgiExtensions.size(); j++)
			{
				if (j + 1 == _configs.locations[i].cgiExtensions.size())
					std::cout << _configs.locations[i].cgiExtensions[j] << std::endl;
				else
					std::cout << _configs.locations[i].cgiExtensions[j] << ", ";
			}
		}
		if (_configs.locations[i].autoIndex)
			std::cout << "\tautoindex: " << "on" << std::endl;
		if (_configs.locations[i].formSubmit)
			std::cout << "\tform_output_file: " << _configs.locations[i].formUploadFile << std::endl;
		std::cout<< "}" << std::endl;		
	}
	std::cout << "}" << std::endl;
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
	if (type == PATH_LOCATION)
		return "PATH_LOCATION)";
	if (type == NUMBER)
		return "NUMBER";
	return "UNKNOWN";
}

void ConfigFileParser::printTokens()
{
	for (size_t i = 0; i < _tokens.size(); i++)
	{
		std::cout << printTokenType(_tokens[i].type) << " => " << _tokens[i].val << std::endl;
	}
}
