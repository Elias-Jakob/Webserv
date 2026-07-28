#include "ConfigFileParser.hpp"

ConfigFileParser::ConfigFileParser(){}

ConfigFileParser::~ConfigFileParser(){}

std::vector<t_Configs>	&ConfigFileParser::parseFile(const std::string &filePath)
{
	// check filename
	std::ifstream	fs(filePath.c_str());
	std::string		str;
	std::string		buffer;
	if (!fs.is_open())
		throw std::runtime_error(std::strerror(errno));
	while (std::getline(fs, buffer)) {
		if (buffer[0] == '#')
			continue ;
		str += buffer + '\n';
	}

	tokenize(str);
	adjustTokens();
	parseToDataStructure();
	if (PRINT_SERVER_CONFIG)
		printServers();
	return (_servers);
}

void ConfigFileParser::tokenize(const std::string &input)
{
	std::cout << "TOKENIZATION" << std::endl;
	size_t start = 0;
	size_t end = 0;
	bool	inToken = false;
	for (size_t i = 0; i < input.size(); i++) {
		if (isValidChar(input[i])) {
			if (!inToken) {
				start = i;
				inToken = true;
			}
		}
		else {
			if (inToken) {
				end = i;
				t_Token	tok;
				tok.val = input.substr(start, end - start);
				tok.type = getTokenType(tok.val);
				_tokens.push_back(tok);
				inToken = false;
				if (input[i] == ',') {
					start = i;
					inToken = true;
				}
			}
			else if ((input[i] != ' ' && input[i] != '\n')&& !inToken)
				std::cout << "SYNTAX_ERROR = " << input[i] << std::endl;
		}
	}
	if (inToken) {
		t_Token	tok;
		tok.val = input.substr(start, input.size() - start);
		tok.type = getTokenType(tok.val);
		_tokens.push_back(tok);
	}
}

bool ConfigFileParser::isValidChar(char c)
{
	if (c == ' ' || c == ';' || c == '\n' || c == ',' || c == '\\' || c == '\t')
		return false;
	if (isalpha(c))
		return true;
	if (c == '{' || c == '}')
		return true;
	if (c == '[' || c == ']')
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
			std::cout << i << std::endl;
			createServer(&i);
		}
	}
}

size_t ConfigFileParser::createServer(size_t *i)
{
	size_t	j;
	t_Configs	serverConfigs;
	*i += 1;
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
			serverConfigs.errorPages[errorCode] = _tokens[j + 1].val;
			j += 1;
		}
		else if (_tokens[j].type == ASSIGN)
		{
			if (checkIdentifier(_tokens[j-1].val))
				setValue(_tokens[j-1].val, j, &serverConfigs);
		}
		else if (_tokens[j].type == LOCATION)
		{
			// std::cout << _tokens[j+1].val << std::endl;
			j += createLocation(j, &serverConfigs);
		}
		else if (_tokens[j].type == SERVER || _tokens[j].type == BRACE_CLOSE)
		{
			std::cout << "SERVER TOKEN" << std::endl;
			break ;
		}
	}
	*i = j;
	parseEndpoints(&serverConfigs);
	_servers.push_back(serverConfigs);
	return j;
}

size_t ConfigFileParser::setLocationVal(size_t i, t_Location *loc)
{
	if (i + 1 >= _tokens.size())
		return i;
	if (_tokens[i - 1].val == "alias")
		loc->alias = _tokens[i + 1].val;
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
		loc->cgi = true;
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
	if (_tokens[i - 1].val == "cgi_path")
	{
		loc->cgi = true;
		loc->cgiPath = _tokens[i + 1].val;
	}
	if (_tokens[i - 1].val == "client_max_body_size") {
		loc->maxBodySize = convertStrToSize(_tokens[i + 1].val);
		loc->sizeIsSet = true;
	}
	return i + 1;
}

size_t ConfigFileParser::createLocation(size_t i, t_Configs *serverConfigs) // add serverConfigs.
{
	t_Location	tempLoc;
	size_t		j = i;
	
	tempLoc.path = _tokens[i+1].val;
	tempLoc.sizeIsSet = false;
	tempLoc.maxBodySize = 0;
	tempLoc.redirect = false;
	tempLoc.upload = false;
	tempLoc.autoIndex = false;
	tempLoc.formSubmit = false;
	tempLoc.cgi = false;
	while (_tokens.size() > j && _tokens[j].type != BRACE_CLOSE && _tokens[j].type != END_OF_FILE)
	{
		if (_tokens[j].type == ASSIGN)
		{
			j = setLocationVal(j, &tempLoc);
			j--;
		}
		j++;
	}
	serverConfigs->locations.push_back(tempLoc);
	_configs.locations.push_back(tempLoc);
	return (j - i);
}

// accepting multiple Ports on one interface map<string, vector<string>>
void	ConfigFileParser::parseEndpoints(t_Configs *serverConfs)
{
	size_t	seperator = 0;
	for (size_t i = 0; i < serverConfs->listenInterfaces.size(); i++)
	{
		if (isIPv6(serverConfs->listenInterfaces[i])) {
			parseIPv6(serverConfs, serverConfs->listenInterfaces[i]);
		}
		else {
			seperator = serverConfs->listenInterfaces[i].find(':', 0);
			if (seperator != std::string::npos) {
				std::string	s = serverConfs->listenInterfaces[i];
				std::string	interface;
				std::string	port;

				interface = s.substr(0, seperator);
				port = s.substr(seperator + 1, s.size() - seperator -1);

				std::map<std::string, std::vector<std::string> >::iterator it = serverConfs->endpoints.find(interface);
				if (it != serverConfs->endpoints.end()) {
					it->second.push_back(port);
				}
				else
					serverConfs->endpoints[interface].push_back(port);//  = port;
			}
		}
	}
}

bool	ConfigFileParser::isIPv6(const std::string &listenInterface)
{
	size_t count = 0;
	for (size_t i = 0; i < listenInterface.size(); i++) {
		if (listenInterface[i] == ':')
			count++;
	}
	if (count > 1)
		return true;
	return false;
}

void	ConfigFileParser::parseIPv6(t_Configs *serverConfs, const std::string &listenInterface)
{
	// std::cout << "parseIPv6()\t" << listenInterface << std::endl;
	size_t	start = listenInterface.find_first_of('[');
	size_t	end = listenInterface.find_last_of(']');
	// std::cout << "start: " << start << ", end: " << end << std::endl;
	std::string	interface = listenInterface.substr(start + 1, end - (start + 1));
	std::string	port = listenInterface.substr(end + 2);
	
	// std::cout << "IPV6\t(" << interface << "):(" << port << ");" << std::endl;

	std::map<std::string, std::vector<std::string> >::iterator it = serverConfs->endpoints.find(interface);
	if (it != serverConfs->endpoints.end())
		it->second.push_back(port);
	else
		serverConfs->endpoints[interface].push_back(port);
}

bool	ConfigFileParser::checkIdentifier(const std::string identifier)
{
	if (identifier == "listen"
		|| identifier == "server_name"
		|| identifier == "error_page"
		|| identifier == "client_max_body_size"
		|| identifier == "root")
		return true;
	return false;
}

// change to set values for this->server.
void ConfigFileParser::setValue(const std::string id, size_t j, t_Configs *serverConfigs)
{
	if (j + 1 >= _tokens.size())
		return ;
	if (id == "listen") {
		serverConfigs->listenInterfaces.push_back(_tokens[j + 1].val);
		std::cout << "\tLISTEN: " << serverConfigs->listenInterfaces.size() << std::endl;
	}
	else if (id == "server_name")
		serverConfigs->serverName = _tokens[j + 1].val;
	else if (id == "client_max_body_size")
		serverConfigs->maxBodySize = convertStrToSize(_tokens[j + 1].val);
	else if (id == "root")
		serverConfigs->root = _tokens[j + 1].val;
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

void ConfigFileParser::printServers()
{
	for(size_t i = 0; i < _servers.size(); i++)
	{
		// std::cout << "Server " << _servers[i].serverName << std::endl;
		printServer(i);
	}
}

void ConfigFileParser::printServer(size_t z)
{
	std::cout << "\n\nSERVER DATA{" << std::endl;
	std::cout << "\tname: "<< _servers[z].serverName << std::endl;
	std::cout << "\tmax_body_size: " << _servers[z].maxBodySize << std::endl;
	std::cout << "\troot: " << _servers[z].root << std::endl;
	std::map<std::string, std::vector<std::string> >::iterator itIP = _servers[z].endpoints.begin();
	std::map<std::string, std::vector<std::string> >::iterator itIPe = _servers[z].endpoints.end();
	std::cout << "\tEndpoints {\n";
	while (itIP != itIPe)
	{
		std::cout << "\t\t[" << itIP->first << "] = ";
		for (size_t i = 0; i < itIP->second.size(); i++)
		{
			std::cout << itIP->second[i] << ", ";
		}
		std::cout << std::endl;
		itIP++;
	}
	std::cout << "\t}" << std::endl;

	std::map<int, std::string>::iterator it = _servers[z].errorPages.begin();
	std::map<int, std::string>::iterator ite = _servers[z].errorPages.end();
	std::cout << "\terrorPages{\n";
	while (it != ite)
	{
		std::cout << "\t\t[" << it->first << "] = " << it->second << std::endl;
		it++;
	}
	std::cout << "\t}" << std::endl;
	for (size_t i = 0; i < _servers[z].locations.size(); i++)
	{
		std::cout << "\tlocation {" << std::endl;
		std::cout << "\t\tpath= " << _servers[z].locations[i].path << std::endl;
		std::cout << "\t\talias= " << _servers[z].locations[i].alias << std::endl;
		if (_servers[z].locations[i].sizeIsSet)
			std::cout << "\t\tmaxBodySize= " << _servers[z].locations[i].maxBodySize << std::endl;
		if (_servers[z].locations[i].defaultPage.size() > 0)
			std::cout << "\t\tindex= " << _servers[z].locations[i].defaultPage << std::endl;
		if (_servers[z].locations[i].allowedMethods.size() > 0)
		{
			std::cout << "\t\tmethods= ";
			for(size_t j = 0; j < _servers[z].locations[i].allowedMethods.size(); j++)
			{
				if (j + 1 == _servers[z].locations[i].allowedMethods.size())
					std::cout << _servers[z].locations[i].allowedMethods[j] << std::endl;
				else
					std::cout << _servers[z].locations[i].allowedMethods[j] << ", ";
			}
		}
		if (_servers[z].locations[i].redirect)
		{
			std::cout << "\t\tredirectCode: " << _servers[z].locations[i].redirectCode << std::endl;
			std::cout << "\t\tredirectURL: " << _servers[z].locations[i].redirectURL << std::endl;
		}
		if (_servers[z].locations[i].upload)
			std::cout << "\t\tuploadStorage: " << _servers[z].locations[i].uploadStore << std::endl;
		if (_servers[z].locations[i].upload)
		{
			std::cout << "\t\textensions = ";
			for (size_t j = 0; j < _servers[z].locations[i].uploadExtensions.size(); j++)
			{
				if (j + 1 == _servers[z].locations[i].uploadExtensions.size())
					std::cout << _servers[z].locations[i].uploadExtensions[j] << std::endl;
				else
					std::cout << _servers[z].locations[i].uploadExtensions[j] << ", ";
			}
		}
		if (_servers[z].locations[i].cgiExtensions.size() > 0)
		{
			std::cout << "\t\tcgiExtensions: ";
			for (size_t j = 0; j < _servers[z].locations[i].cgiExtensions.size(); j++)
			{
				if (j + 1 == _servers[z].locations[i].cgiExtensions.size())
					std::cout << _servers[z].locations[i].cgiExtensions[j] << std::endl;
				else
					std::cout << _servers[z].locations[i].cgiExtensions[j] << ", ";
			}
		}
		if (_servers[z].locations[i].autoIndex)
			std::cout << "\t\tautoindex: " << "on" << std::endl;
		if (_servers[z].locations[i].formSubmit)
			std::cout << "\t\tform_output_file: " << _servers[z].locations[i].formUploadFile << std::endl;
		if (_servers[z].locations[i].cgi)
			std::cout << "\t\tcgi_path: " << _servers[z].locations[i].cgiPath << std::endl;
		std::cout<< "\t}" << std::endl;		
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
